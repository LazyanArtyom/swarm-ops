#include "dialogs/settings_dialog.h"

#include <QAbstractItemView>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "configs/app_configs.h"
#include "dialogs/message_dialog.h"
#include "object_names.h"
#include "theme/theme_manager.h"
#include "theme/theme_metrics.h"
#include "widgets/settings_form_row.h"

namespace app::ui {
namespace {

constexpr auto kEnglishLanguageCode = "en";
constexpr auto kRussianLanguageCode = "ru";

}  // namespace

SettingsDialog::SettingsDialog(configs::AppSettings& settings, QWidget* parent)
    : QDialog(parent), settings_(settings) {
    setObjectName(QString::fromLatin1(object_names::kSettingsDialog));
    setWindowTitle(tr("Settings"));
    setModal(true);
    const auto metrics = theme::ThemeMetrics::Instance().Current();
    setMinimumSize(metrics.settings_dialog_min_width_px, metrics.settings_dialog_min_height_px);

    auto* root_layout = new QVBoxLayout(this);
    root_layout->setContentsMargins(metrics.spacing_lg_px, metrics.spacing_lg_px,
                                    metrics.spacing_lg_px, metrics.spacing_md_px);
    root_layout->setSpacing(metrics.spacing_md_px);

    auto* content_layout = new QHBoxLayout();
    content_layout->setSpacing(metrics.spacing_md_px);

    navigation_ = new QListWidget(this);
    navigation_->setObjectName(QString::fromLatin1(object_names::kSettingsNavigation));
    navigation_->setFixedWidth(metrics.settings_navigation_width_px);
    navigation_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    pages_ = new QStackedWidget(this);
    pages_->setObjectName(QString::fromLatin1(object_names::kSettingsContent));

    AddPage(tr("General"), CreateGeneralPage());
    AddPage(tr("Appearance"), CreateAppearancePage());

    content_layout->addWidget(navigation_);
    content_layout->addWidget(pages_, 1);
    root_layout->addLayout(content_layout, 1);

    buttons_ = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, this);
    save_button_ = buttons_->button(QDialogButtonBox::Save);
    if (save_button_ != nullptr) {
        save_button_->setEnabled(false);
        save_button_->setDefault(true);
    }
    root_layout->addWidget(buttons_);

    connect(navigation_, &QListWidget::currentRowChanged, pages_, &QStackedWidget::setCurrentIndex);
    connect(buttons_, &QDialogButtonBox::accepted, this, &SettingsDialog::SaveSettings);
    connect(buttons_, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    connect(language_combo_, &QComboBox::currentIndexChanged, this,
            &SettingsDialog::RefreshDirtyState);
    connect(theme_combo_, &QComboBox::currentIndexChanged, this, &SettingsDialog::RefreshDirtyState);

    LoadSettings();
    navigation_->setCurrentRow(0);
    RefreshDirtyState();
}

bool SettingsDialog::LanguageChanged() const {
    return language_changed_;
}

void SettingsDialog::SaveSettings() {
    language_changed_ = SelectedLanguageCode() != initial_language_code_;
    const bool theme_changed = SelectedDarkTheme() != initial_dark_theme_;

    settings_.General().SetLanguageCode(SelectedLanguageCode());
    settings_.Theme().SetDarkTheme(SelectedDarkTheme());

    if (theme_changed) {
        theme::ThemeManager::Instance().Apply(SelectedDarkTheme() ? theme::ThemeId::kDark
                                                                  : theme::ThemeId::kLight);
    }

    const app::VoidResult<QString> sync_result = settings_.Sync();
    if (!sync_result) {
        MessageDialog::ShowError(this, tr("Settings Error"),
                                 tr("Settings could not be saved."),
                                 sync_result.error());
        return;
    }

    accept();
}

void SettingsDialog::RefreshDirtyState() {
    const bool dirty = SelectedLanguageCode() != initial_language_code_ ||
                       SelectedDarkTheme() != initial_dark_theme_;
    if (save_button_ != nullptr) {
        save_button_->setEnabled(dirty);
    }
    if (restart_hint_ != nullptr) {
        restart_hint_->setVisible(SelectedLanguageCode() != initial_language_code_);
    }
}

QWidget* SettingsDialog::CreateGeneralPage() {
    const auto metrics = theme::ThemeMetrics::Instance().Current();
    auto* page = new QWidget(this);
    page->setObjectName(QString::fromLatin1(object_names::kSettingsPage));

    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(metrics.spacing_lg_px, metrics.spacing_lg_px, metrics.spacing_lg_px,
                               metrics.spacing_lg_px);
    layout->setSpacing(metrics.spacing_md_px);

    language_combo_ = new QComboBox(page);
    language_combo_->addItem(tr("English"), QString::fromLatin1(kEnglishLanguageCode));
    language_combo_->addItem(tr("Russian"), QString::fromLatin1(kRussianLanguageCode));
    language_row_ = new SettingsFormRow(tr("Language"), language_combo_, page);
    language_row_->ApplyMetrics(metrics);
    layout->addWidget(language_row_);

    restart_hint_ = new QLabel(tr("Language changes are applied after restart."), page);
    restart_hint_->setObjectName(QString::fromLatin1(object_names::kSettingsRestartHint));
    restart_hint_->setProperty("role", QStringLiteral("muted"));
    restart_hint_->setWordWrap(true);
    restart_hint_->setVisible(false);
    layout->addWidget(restart_hint_);
    layout->addStretch(1);
    return page;
}

QWidget* SettingsDialog::CreateAppearancePage() {
    const auto metrics = theme::ThemeMetrics::Instance().Current();
    auto* page = new QWidget(this);
    page->setObjectName(QString::fromLatin1(object_names::kSettingsPage));

    auto* layout = new QVBoxLayout(page);
    layout->setContentsMargins(metrics.spacing_lg_px, metrics.spacing_lg_px, metrics.spacing_lg_px,
                               metrics.spacing_lg_px);
    layout->setSpacing(metrics.spacing_md_px);

    theme_combo_ = new QComboBox(page);
    theme_combo_->addItem(tr("Light"), false);
    theme_combo_->addItem(tr("Dark"), true);
    theme_row_ = new SettingsFormRow(tr("Theme"), theme_combo_, page);
    theme_row_->ApplyMetrics(metrics);
    layout->addWidget(theme_row_);
    layout->addStretch(1);
    return page;
}

void SettingsDialog::AddPage(const QString& title, QWidget* page) {
    navigation_->addItem(title);
    pages_->addWidget(page);
}

void SettingsDialog::LoadSettings() {
    initial_language_code_ = settings_.General().LanguageCode();
    initial_dark_theme_ = settings_.Theme().DarkTheme();

    const int language_index = language_combo_->findData(initial_language_code_);
    language_combo_->setCurrentIndex(language_index >= 0 ? language_index : 0);
    const int theme_index = theme_combo_->findData(initial_dark_theme_);
    theme_combo_->setCurrentIndex(theme_index >= 0 ? theme_index : 0);
}

QString SettingsDialog::SelectedLanguageCode() const {
    return language_combo_ != nullptr ? language_combo_->currentData().toString()
                                      : QString::fromLatin1(kEnglishLanguageCode);
}

bool SettingsDialog::SelectedDarkTheme() const {
    return theme_combo_ != nullptr && theme_combo_->currentData().toBool();
}

}  // namespace app::ui
