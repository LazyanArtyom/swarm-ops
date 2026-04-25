#pragma once

#include <QDialog>

class QComboBox;
class QDialogButtonBox;
class QLabel;
class QListWidget;
class QPushButton;
class QStackedWidget;
class QWidget;

namespace app::configs {
class AppSettings;
}

namespace app::ui {

class SettingsFormRow;

class SettingsDialog final : public QDialog {
    Q_OBJECT

   public:
    explicit SettingsDialog(configs::AppSettings& settings, QWidget* parent = nullptr);
    ~SettingsDialog() override = default;

    [[nodiscard]] bool LanguageChanged() const;

   private slots:
    void SaveSettings();
    void RefreshDirtyState();

   private:
    QWidget* CreateGeneralPage();
    QWidget* CreateAppearancePage();
    void AddPage(const QString& title, QWidget* page);
    void LoadSettings();

    [[nodiscard]] QString SelectedLanguageCode() const;
    [[nodiscard]] bool SelectedDarkTheme() const;

    configs::AppSettings& settings_;
    QListWidget* navigation_{nullptr};
    QStackedWidget* pages_{nullptr};
    QComboBox* language_combo_{nullptr};
    QComboBox* theme_combo_{nullptr};
    SettingsFormRow* language_row_{nullptr};
    SettingsFormRow* theme_row_{nullptr};
    QLabel* restart_hint_{nullptr};
    QDialogButtonBox* buttons_{nullptr};
    QPushButton* save_button_{nullptr};
    QString initial_language_code_;
    bool initial_dark_theme_{false};
    bool language_changed_{false};
};

}  // namespace app::ui
