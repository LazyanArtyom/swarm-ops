#include "panels/central_panel.h"

#include <QStackedWidget>
#include <QTabWidget>
#include <QVBoxLayout>

#include "object_names.h"
#include "workspace/page_host.h"

namespace app::ui {

CentralPanel::CentralPanel(QWidget* parent) : QWidget(parent) {
    setObjectName(QString::fromLatin1(object_names::kCentralPanel));

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    stacked_ = new QStackedWidget(this);
    stacked_->setObjectName(QString::fromLatin1(object_names::kCentralPanelStack));
    tabs_ = new QTabWidget(this);
    tabs_->setObjectName(QString::fromLatin1(object_names::kCentralPanelTabs));
    page_host_ = new workspace::PageHost(stacked_, tabs_, this);

    stacked_->setFrameShape(QFrame::NoFrame);
    tabs_->setDocumentMode(true);
    tabs_->setTabsClosable(true);
    tabs_->setMovable(true);

    connect(tabs_, &QTabWidget::tabCloseRequested, this, [this](int index) { CloseTab(index); });
    connect(page_host_, &workspace::PageHost::SigCurrentPageChanged, this,
            &CentralPanel::SigCurrentPageChanged);

    layout->addWidget(stacked_);
    layout->addWidget(tabs_);

    SetMode(Mode::kSingle);
}

void CentralPanel::SetMode(Mode mode) {
    if (mode_ == mode) {
        RebuildForMode(mode_);
        return;
    }
    mode_ = mode;
    RebuildForMode(mode_);
}

CentralPanel::Mode CentralPanel::CurrentMode() const {
    return mode_;
}

void CentralPanel::RegisterPageFactory(const QString& page_key, PageFactory factory) {
    if (page_key.isEmpty() || !factory) {
        return;
    }
    page_registry_->RegisterFactory(page_key, std::move(factory));
}

void CentralPanel::UnregisterPageFactory(const QString& page_key) {
    page_registry_->UnregisterFactory(page_key);
}

void CentralPanel::SetPageRegistry(workspace::PageRegistry* page_registry) {
    page_registry_ = page_registry != nullptr ? page_registry : &owned_page_registry_;
}

QByteArray CentralPanel::SaveSession() const {
    return page_host_ != nullptr ? page_host_->SaveSession() : QByteArray();
}

bool CentralPanel::RestoreSession(const QByteArray& state) {
    if (page_host_ == nullptr || state.isEmpty()) {
        return false;
    }

    const bool restored = page_host_->RestoreSession(state, *page_registry_);
    if (restored) {
        mode_ = page_host_->CurrentMode() == workspace::PageHostMode::kTabs ? Mode::kTabs
                                                                            : Mode::kSingle;
        UpdateBackSignal();
    }
    return restored;
}

QWidget* CentralPanel::ShowPage(const QString& page_key) {
    return OpenPage(page_key, DefaultOpenOptions(page_key));
}

QWidget* CentralPanel::OpenPage(const QString& page_key, const OpenOptions& options) {
    QWidget* page =
        page_host_ != nullptr ? page_host_->EnsurePage(page_key, *page_registry_) : nullptr;
    if (page == nullptr) {
        return nullptr;
    }

    if (options.open_mode == workspace::PageOpenMode::kTab) {
        return ActivateTab(page_key, page, options);
    }

    ActivateSingle(page_key, page);
    return page;
}

QWidget* CentralPanel::PushPage(const QString& page_key) {
    if (mode_ != Mode::kSingle) {
        // In tab mode, "push" behaves like opening/activating tab.
        OpenOptions opt;
        opt.open_mode = workspace::PageOpenMode::kTab;
        return OpenPage(page_key, opt);
    }

    const QString current_page_key = CurrentPageKey();
    if (!current_page_key.isEmpty()) {
        nav_stack_.push_back(current_page_key);
    }

    QWidget* page =
        page_host_ != nullptr ? page_host_->EnsurePage(page_key, *page_registry_) : nullptr;
    if (page == nullptr) {
        UpdateBackSignal();
        return nullptr;
    }

    ActivateSingle(page_key, page);
    UpdateBackSignal();
    return page;
}

void CentralPanel::PopPage() {
    if (mode_ != Mode::kSingle) {
        return;
    }
    if (nav_stack_.isEmpty()) {
        UpdateBackSignal();
        return;
    }

    const QString previous_key = nav_stack_.takeLast();
    QWidget* page =
        page_host_ != nullptr ? page_host_->EnsurePage(previous_key, *page_registry_) : nullptr;
    if (page != nullptr) {
        ActivateSingle(previous_key, page);
    }
    UpdateBackSignal();
}

bool CentralPanel::CanGoBack() const {
    return (mode_ == Mode::kSingle) && !nav_stack_.isEmpty();
}

QString CentralPanel::CurrentPageKey() const {
    return page_host_ != nullptr ? page_host_->CurrentPageKey() : QString();
}

void CentralPanel::ActivateSingle(const QString& page_key, QWidget* page) {
    if (page_host_ != nullptr) {
        page_host_->ActivateSingle(page_key, page);
    }
    UpdateBackSignal();
}

QWidget* CentralPanel::ActivateTab(const QString& page_key, QWidget* page,
                                   const OpenOptions& options) {
    const workspace::PageDescriptor descriptor = page_registry_->Descriptor(page_key);
    workspace::PageHost::TabOptions tab_options{
        .title = options.title.isEmpty() ? descriptor.title : options.title,
        .icon = descriptor.icon,
        .single_instance = options.single_instance,
        .closable = descriptor.closable,
    };
    QWidget* active_page = page_host_ != nullptr ? page_host_->ActivateTab(page_key, page, tab_options)
                                                 : page;
    UpdateBackSignal();
    return active_page;
}

void CentralPanel::RebuildForMode(Mode mode) {
    if (page_host_ == nullptr) {
        return;
    }

    page_host_->SetMode(mode == Mode::kTabs ? workspace::PageHostMode::kTabs
                                            : workspace::PageHostMode::kSingle);
    UpdateBackSignal();
}

void CentralPanel::UpdateBackSignal() {
    emit SigBackAvailabilityChanged(CanGoBack());
}

CentralPanel::OpenOptions CentralPanel::DefaultOpenOptions(const QString& page_key) const {
    const workspace::PageDescriptor descriptor = page_registry_->Descriptor(page_key);
    return {
        .open_mode = mode_ == Mode::kTabs ? workspace::PageOpenMode::kTab
                                          : descriptor.default_open_mode,
        .single_instance = true,
        .title = descriptor.title,
    };
}

void CentralPanel::CloseTab(int index) {
    if (page_host_ == nullptr) {
        return;
    }

    page_host_->CloseTab(index);
}

}  // namespace app::ui
