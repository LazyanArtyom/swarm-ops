#pragma once

#include <QByteArray>
#include <QSettings>
#include <QString>
#include <QVariant>
#include <cstdint>
#include <functional>
#include <span>

#include "logging/logger.h"
#include "utils/result.h"

namespace app::configs {

inline constexpr int kCurrentSettingsSchemaVersion = 1;

enum class SettingsImportMode : std::uint8_t {
    kMerge,
    kReplace,
};

template <typename Value>
struct SettingsKey final {
    const char* group;
    const char* name;
};

struct SettingsMigrationStep final {
    int from_version{0};
    int to_version{0};
    QString description;
    std::function<app::VoidResult<QString>(QSettings& settings)> migrate;
};

class WindowSettings final {
   public:
    void SaveMainWindowGeometry(const QByteArray& geometry) const;
    [[nodiscard]] QByteArray LoadMainWindowGeometry() const;

    void SaveWorkspaceState(const QByteArray& state) const;
    [[nodiscard]] QByteArray LoadWorkspaceState() const;

    void SaveMainWindowLayoutState(const QString& layout_id, const QByteArray& state) const;
    [[nodiscard]] QByteArray LoadMainWindowLayoutState(const QString& layout_id) const;
    void SavePageSessionState(const QString& layout_id, const QByteArray& state) const;
    [[nodiscard]] QByteArray LoadPageSessionState(const QString& layout_id) const;

    void Reset() const;

   private:
    friend class AppSettings;

    explicit WindowSettings(QSettings& settings);

    QSettings& settings_;
};

class ThemeSettings final {
   public:
    [[nodiscard]] bool DarkTheme() const;
    void SetDarkTheme(bool enabled) const;
    void Reset() const;

   private:
    friend class AppSettings;

    explicit ThemeSettings(QSettings& settings);

    QSettings& settings_;
};

class GeneralSettings final {
   public:
    [[nodiscard]] QString LanguageCode() const;
    void SetLanguageCode(const QString& language_code) const;
    void Reset() const;

   private:
    friend class AppSettings;

    explicit GeneralSettings(QSettings& settings);

    QSettings& settings_;
};

class GenericSettings final {
   public:
    void SetValue(const QString& dotted_key, const QVariant& value) const;
    [[nodiscard]] QVariant Value(const QString& dotted_key, const QVariant& def = QVariant()) const;
    void RemoveValue(const QString& dotted_key) const;

   private:
    friend class AppSettings;

    explicit GenericSettings(QSettings& settings);

    QSettings& settings_;
};

class LoggingSettings final {
   public:
    [[nodiscard]] app::logging::LoggerConfig LoadLoggerConfig(
        const app::logging::LoggerConfig& defaults) const;
    void SaveLoggerConfig(const app::logging::LoggerConfig& config) const;
    void Reset() const;

   private:
    friend class AppSettings;

    explicit LoggingSettings(QSettings& settings);

    QSettings& settings_;
};

class AppSettings final {
   public:
    AppSettings();
    AppSettings(QSettings::Format format, QSettings::Scope scope, const QString& organization,
                const QString& application = QString());
    AppSettings(const AppSettings&) = delete;
    AppSettings& operator=(const AppSettings&) = delete;
    AppSettings(AppSettings&&) = delete;
    AppSettings& operator=(AppSettings&&) = delete;

    [[nodiscard]] static AppSettings& Default();

    [[nodiscard]] WindowSettings& Window();
    [[nodiscard]] GeneralSettings& General();
    [[nodiscard]] ThemeSettings& Theme();
    [[nodiscard]] LoggingSettings& Logging();
    [[nodiscard]] GenericSettings& Generic();

    [[nodiscard]] int SchemaVersion() const;
    [[nodiscard]] QString FileName() const;

    [[nodiscard]] app::VoidResult<QString> Migrate();
    [[nodiscard]] app::VoidResult<QString> Migrate(
        std::span<const SettingsMigrationStep> migrations);
    [[nodiscard]] app::VoidResult<QString> Migrate(
        int target_schema_version, std::span<const SettingsMigrationStep> migrations);
    void ResetToDefaults();
    [[nodiscard]] app::VoidResult<QString> ExportToIni(const QString& path) const;
    [[nodiscard]] app::VoidResult<QString> ImportFromIni(
        const QString& path, SettingsImportMode mode = SettingsImportMode::kMerge);
    [[nodiscard]] app::VoidResult<QString> Sync() const;

   private:
    void SetSchemaVersion(int version) const;

    QSettings settings_;
    WindowSettings window_;
    GeneralSettings general_;
    ThemeSettings theme_;
    LoggingSettings logging_;
    GenericSettings generic_;
};

}  // namespace app::configs
