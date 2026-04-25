#include "workspace/page_host.h"

#include <QDataStream>
#include <QIODevice>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QTabBar>
#include <QTabWidget>
#include <QWidget>
#include <optional>
#include <type_traits>
#include <utility>

#include "workspace/page_lifecycle.h"
#include "workspace/page_registry.h"

namespace app::ui::workspace {
namespace {

constexpr auto kPageSessionMagic = "page-host-v1";
constexpr QDataStream::Version kPageSessionStreamVersion = QDataStream::Qt_6_0;

IPageLifecycle* Lifecycle(QWidget* page) {
    return dynamic_cast<IPageLifecycle*>(page);
}

std::optional<PageHost::SessionState> DeserializeSession(const QByteArray& bytes) {
    QDataStream stream(bytes);
    stream.setVersion(kPageSessionStreamVersion);

    QString magic;
    std::underlying_type_t<PageHostMode> raw_mode{};
    PageHost::SessionState session;
    qint32 entry_count = 0;

    stream >> magic >> raw_mode >> session.current_page_key >> session.current_page_state >>
        session.current_tab_index >> entry_count;

    if (stream.status() != QDataStream::Ok || magic != QString::fromLatin1(kPageSessionMagic)) {
        return std::nullopt;
    }

    session.mode = raw_mode == static_cast<std::underlying_type_t<PageHostMode>>(PageHostMode::kTabs)
                       ? PageHostMode::kTabs
                       : PageHostMode::kSingle;
    for (qint32 index = 0; index < entry_count; ++index) {
        PageHost::SessionTabEntry entry;
        stream >> entry.page_key >> entry.title >> entry.state;
        session.tabs.push_back(std::move(entry));
    }

    if (stream.status() != QDataStream::Ok) {
        return std::nullopt;
    }

    return session;
}

QByteArray SerializeSession(const PageHost::SessionState& session) {
    QByteArray bytes;
    QDataStream stream(&bytes, QIODevice::WriteOnly);
    stream.setVersion(kPageSessionStreamVersion);

    stream << QString::fromLatin1(kPageSessionMagic);
    stream << static_cast<std::underlying_type_t<PageHostMode>>(session.mode);
    stream << session.current_page_key;
    stream << session.current_page_state;
    stream << session.current_tab_index;
    stream << static_cast<qint32>(session.tabs.size());
    for (const PageHost::SessionTabEntry& entry : session.tabs) {
        stream << entry.page_key << entry.title << entry.state;
    }

    return bytes;
}

}  // namespace

PageHost::PageHost(QStackedWidget* stacked, QTabWidget* tabs, QObject* parent)
    : QObject(parent), stacked_(stacked), tabs_(tabs) {
    if (tabs_ != nullptr) {
        connect(tabs_, &QTabWidget::currentChanged, this, &PageHost::OnCurrentTabChanged);
    }
}

QWidget* PageHost::EnsurePage(const QString& page_key, const PageRegistry& registry) {
    if (page_key.isEmpty()) {
        return nullptr;
    }

    if (pages_.contains(page_key) && !pages_[page_key].isNull()) {
        return pages_[page_key].data();
    }

    QWidget* page = registry.CreatePage(page_key, stacked_);
    if (page == nullptr) {
        return nullptr;
    }

    page->setObjectName(page_key);
    TrackPage(page_key, page);
    return page;
}

void PageHost::SetMode(PageHostMode mode) {
    if (stacked_ == nullptr || tabs_ == nullptr) {
        return;
    }

    mode_ = mode;
    if (mode == PageHostMode::kTabs) {
        tabs_->show();
        stacked_->hide();
        return;
    }

    stacked_->show();
    tabs_->hide();
}

void PageHost::ActivateSingle(const QString& page_key, QWidget* page) {
    if (page == nullptr || stacked_ == nullptr || tabs_ == nullptr) {
        return;
    }

    const int tab_index = TabIndexOf(page);
    if (tab_index >= 0) {
        tabs_->removeTab(tab_index);
    }

    if (stacked_->indexOf(page) < 0) {
        stacked_->addWidget(page);
    }

    stacked_->setCurrentWidget(page);
    SetMode(PageHostMode::kSingle);
    TrackPage(page_key, page);
    SetCurrentPage(page_key, page);
}

QWidget* PageHost::ActivateTab(const QString& page_key, QWidget* page,
                               const TabOptions& options) {
    if (page == nullptr || stacked_ == nullptr || tabs_ == nullptr) {
        return nullptr;
    }

    const int existing_tab_index = TabIndexOf(page);
    if (existing_tab_index >= 0) {
        tabs_->setCurrentIndex(existing_tab_index);
        return page;
    }

    if (options.single_instance && pages_.contains(page_key) && !pages_[page_key].isNull() &&
        pages_[page_key].data() != page) {
        page = pages_[page_key].data();
    }

    const int stacked_index = stacked_->indexOf(page);
    if (stacked_index >= 0) {
        stacked_->removeWidget(page);
    }

    const QString tab_title = options.title.isEmpty() ? page_key : options.title;
    const int tab_index = tabs_->addTab(page, options.icon, tab_title);
    tab_closable_[page] = options.closable;
    if (!options.closable && tabs_->tabBar() != nullptr) {
        tabs_->tabBar()->setTabButton(tab_index, QTabBar::RightSide, nullptr);
    }
    tabs_->setCurrentIndex(tab_index);
    SetMode(PageHostMode::kTabs);
    TrackPage(page_key, page);
    SetCurrentPage(page_key, page);
    return page;
}

void PageHost::CloseTab(int index) {
    if (tabs_ == nullptr || index < 0 || index >= tabs_->count()) {
        return;
    }

    QWidget* page = tabs_->widget(index);
    const QString page_key = KeyForWidget(page);
    if (!CanClosePage(page_key, page)) {
        return;
    }

    tabs_->removeTab(index);

    if (page != nullptr) {
        RemoveTrackedPage(page_key, page);
        page->deleteLater();
    }
}

QByteArray PageHost::SaveSession() const {
    SessionState session;
    session.mode = mode_;
    session.current_page_key = current_page_key_;
    session.current_page_state = SavePageState(current_page_);
    session.current_tab_index = tabs_ != nullptr ? tabs_->currentIndex() : -1;

    if (tabs_ != nullptr) {
        for (int index = 0; index < tabs_->count(); ++index) {
            QWidget* page = tabs_->widget(index);
            session.tabs.push_back({
                .page_key = KeyForWidget(page),
                .title = tabs_->tabText(index),
                .state = SavePageState(page),
            });
        }
    }

    return SerializeSession(session);
}

bool PageHost::RestoreSession(const QByteArray& state, const PageRegistry& registry) {
    const std::optional<SessionState> session = DeserializeSession(state);
    if (!session.has_value()) {
        return false;
    }

    ClearTrackedPages();

    if (session->mode == PageHostMode::kTabs) {
        SetMode(PageHostMode::kTabs);
        for (const SessionTabEntry& entry : session->tabs) {
            QWidget* page = EnsurePage(entry.page_key, registry);
            if (page == nullptr) {
                continue;
            }

            RestorePageState(page, entry.state);
            ActivateTab(entry.page_key, page,
                        {
                            .title = entry.title,
                            .icon = registry.Descriptor(entry.page_key).icon,
                            .single_instance = true,
                            .closable = registry.Descriptor(entry.page_key).closable,
                        });
        }

        if (tabs_ != nullptr && session->current_tab_index >= 0 &&
            session->current_tab_index < tabs_->count()) {
            tabs_->setCurrentIndex(session->current_tab_index);
        }
        return true;
    }

    SetMode(PageHostMode::kSingle);
    QWidget* page = EnsurePage(session->current_page_key, registry);
    if (page == nullptr) {
        return false;
    }

    RestorePageState(page, session->current_page_state);
    ActivateSingle(session->current_page_key, page);
    return true;
}

QString PageHost::KeyForTab(int index) const {
    if (tabs_ == nullptr || index < 0 || index >= tabs_->count()) {
        return {};
    }
    return KeyForWidget(PageForTab(index));
}

QWidget* PageHost::PageForKey(const QString& page_key) const {
    if (page_key.isEmpty() || !pages_.contains(page_key) || pages_[page_key].isNull()) {
        return nullptr;
    }
    return pages_[page_key].data();
}

QWidget* PageHost::PageForTab(int index) const {
    if (tabs_ == nullptr || index < 0 || index >= tabs_->count()) {
        return nullptr;
    }
    return tabs_->widget(index);
}

QString PageHost::KeyForWidget(QWidget* page) const {
    if (page == nullptr) {
        return {};
    }
    return page_keys_.value(page);
}

QString PageHost::CurrentPageKey() const {
    return current_page_key_;
}

QWidget* PageHost::CurrentPage() const {
    return current_page_;
}

PageHostMode PageHost::CurrentMode() const {
    return mode_;
}

void PageHost::SetCurrentPage(const QString& page_key, QWidget* page) {
    if (current_page_key_ == page_key && current_page_ == page) {
        return;
    }

    NotifyDeactivated(current_page_);
    current_page_key_ = page_key;
    current_page_ = page;
    NotifyActivated(current_page_);
    emit SigCurrentPageChanged(current_page_key_);
}

void PageHost::OnCurrentTabChanged(int index) {
    if (mode_ != PageHostMode::kTabs) {
        return;
    }

    QWidget* page = PageForTab(index);
    SetCurrentPage(KeyForWidget(page), page);
}

bool PageHost::CanClosePage(const QString& page_key, QWidget* page) const {
    if (page == nullptr) {
        return true;
    }

    if (tab_closable_.contains(page) && !tab_closable_.value(page)) {
        return false;
    }

    const auto* lifecycle = dynamic_cast<const IPageLifecycle*>(page);
    if (lifecycle != nullptr && !lifecycle->CanClose()) {
        return false;
    }

    Q_UNUSED(page_key);
    return true;
}

void PageHost::ClearTrackedPages() {
    NotifyDeactivated(current_page_);

    if (tabs_ != nullptr) {
        const QSignalBlocker blocker(tabs_);
        while (tabs_->count() > 0) {
            QWidget* page = tabs_->widget(0);
            tabs_->removeTab(0);
            if (page != nullptr) {
                RemoveTrackedPage(KeyForWidget(page), page);
                page->deleteLater();
            }
        }
    }

    if (stacked_ != nullptr) {
        while (stacked_->count() > 0) {
            QWidget* page = stacked_->widget(0);
            stacked_->removeWidget(page);
            if (page != nullptr) {
                RemoveTrackedPage(KeyForWidget(page), page);
                page->deleteLater();
            }
        }
    }

    current_page_.clear();
    current_page_key_.clear();
}

void PageHost::TrackPage(const QString& page_key, QWidget* page) {
    if (page_key.isEmpty() || page == nullptr) {
        return;
    }

    const bool already_tracked = page_keys_.contains(page);
    pages_[page_key] = page;
    page_keys_[page] = page_key;

    if (already_tracked) {
        return;
    }

    connect(page, &QObject::destroyed, this,
            [this, page_key, page] { RemoveTrackedPage(page_key, page); });
}

void PageHost::RemoveTrackedPage(const QString& page_key, QWidget* page) {
    if (!page_key.isEmpty() && pages_.contains(page_key) && pages_[page_key].data() == page) {
        pages_.remove(page_key);
    }
    page_keys_.remove(page);
    tab_closable_.remove(page);
}

int PageHost::TabIndexOf(QWidget* page) const {
    if (tabs_ == nullptr || page == nullptr) {
        return -1;
    }

    return tabs_->indexOf(page);
}

QByteArray PageHost::SavePageState(QWidget* page) {
    if (auto* lifecycle = Lifecycle(page); lifecycle != nullptr) {
        return lifecycle->SaveState();
    }
    return {};
}

void PageHost::RestorePageState(QWidget* page, const QByteArray& state) {
    if (auto* lifecycle = Lifecycle(page); lifecycle != nullptr) {
        lifecycle->RestoreState(state);
    }
}

void PageHost::NotifyActivated(QWidget* page) {
    if (auto* lifecycle = Lifecycle(page); lifecycle != nullptr) {
        lifecycle->OnActivate();
    }
}

void PageHost::NotifyDeactivated(QWidget* page) {
    if (auto* lifecycle = Lifecycle(page); lifecycle != nullptr) {
        lifecycle->OnDeactivate();
    }
}

}  // namespace app::ui::workspace
