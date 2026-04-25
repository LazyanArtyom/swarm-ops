#include "configs/app_configs.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <array>
#include <expected>
#include <ranges>

namespace app::configs {
namespace {

// Groups
constexpr auto kGrpApp = "app";
constexpr auto kGrpGeneral = "general";
constexpr auto kGrpMainWindow = "mainwindow";
constexpr auto kGrpUI = "ui";
constexpr auto kGrpLogging = "logging";

// App keys (under kGrpApp)
constexpr auto kKeySchemaVersion = "schema_version";

// General keys (under kGrpGeneral)
constexpr auto kKeyLanguage = "language";
constexpr auto kDefaultLanguageCode = "en";

// MainWindow keys (under kGrpMainWindow)
constexpr auto kKeyGeometry = "geometry";
constexpr auto kKeyWorkspaceState = "workspace_state";
constexpr auto kGrpLayouts = "layouts";
constexpr auto kDefaultLayoutId = "default";
constexpr auto kKeyLayoutState = "state";
constexpr auto kKeyPageSessionState = "page_session";

// UI keys (under kGrpUI)
constexpr auto kKeyTheme = "theme";

// Logging keys (under kGrpLogging)
constexpr auto kKeySinkType = "sink_type";
constexpr auto kKeyDefaultLevel = "default_level";
constexpr auto kKeyTerminalLevel = "terminal_level";
constexpr auto kKeyFileLevel = "file_level";
constexpr auto kKeyConsoleLevel = "console_level";
constexpr auto kKeyMemoryLevel = "memory_level";
constexpr auto kKeyFlushLevel = "flush_level";
constexpr auto kKeyLogFilePath = "file_path";
constexpr auto kKeyMaxFileSizeBytes = "file_max_size_bytes";
constexpr auto kKeyMaxFiles = "file_max_count";
constexpr auto kKeyMemoryMaxRecords = "memory_max_records";
constexpr auto kKeyIncludeThreadId = "include_thread_id";

constexpr SettingsKey<int> kSchemaVersionKey{.group = kGrpApp, .name = kKeySchemaVersion};
constexpr SettingsKey<QString> kLanguageKey{.group = kGrpGeneral, .name = kKeyLanguage};
constexpr SettingsKey<QByteArray> kMainWindowGeometryKey{.group = kGrpMainWindow,
                                                         .name = kKeyGeometry};
constexpr SettingsKey<QByteArray> kLegacyWorkspaceStateKey{.group = kGrpMainWindow,
                                                           .name = kKeyWorkspaceState};
constexpr SettingsKey<bool> kThemeDarkKey{.group = kGrpUI, .name = kKeyTheme};
constexpr SettingsKey<QString> kLogSinkTypeKey{.group = kGrpLogging, .name = kKeySinkType};
constexpr SettingsKey<QString> kLogDefaultLevelKey{.group = kGrpLogging, .name = kKeyDefaultLevel};
constexpr SettingsKey<QString> kLogTerminalLevelKey{.group = kGrpLogging,
                                                    .name = kKeyTerminalLevel};
constexpr SettingsKey<QString> kLogFileLevelKey{.group = kGrpLogging, .name = kKeyFileLevel};
constexpr SettingsKey<QString> kLogConsoleLevelKey{.group = kGrpLogging, .name = kKeyConsoleLevel};
constexpr SettingsKey<QString> kLogMemoryLevelKey{.group = kGrpLogging, .name = kKeyMemoryLevel};
constexpr SettingsKey<QString> kLogFlushLevelKey{.group = kGrpLogging, .name = kKeyFlushLevel};
constexpr SettingsKey<QString> kLogFilePathKey{.group = kGrpLogging, .name = kKeyLogFilePath};
constexpr SettingsKey<qulonglong> kLogMaxFileSizeBytesKey{.group = kGrpLogging,
                                                          .name = kKeyMaxFileSizeBytes};
constexpr SettingsKey<uint> kLogMaxFilesKey{.group = kGrpLogging, .name = kKeyMaxFiles};
constexpr SettingsKey<qulonglong> kLogMemoryMaxRecordsKey{.group = kGrpLogging,
                                                          .name = kKeyMemoryMaxRecords};
constexpr SettingsKey<bool> kLogIncludeThreadIdKey{.group = kGrpLogging,
                                                   .name = kKeyIncludeThreadId};

class GroupGuard final {
   public:
    GroupGuard(QSettings& configs, const char* grp) : configs_(configs) {
        configs_.beginGroup(QString::fromLatin1(grp));
    }
    GroupGuard(QSettings& configs, const QString& grp) : configs_(configs) {
        configs_.beginGroup(grp);
    }
    ~GroupGuard() {
        configs_.endGroup();
    }
    GroupGuard(const GroupGuard&) = delete;
    GroupGuard& operator=(const GroupGuard&) = delete;

   private:
    QSettings& configs_;
};

QString NormalizedLayoutId(const QString& layout_id) {
    if (layout_id.trimmed().isEmpty()) {
        return QString::fromLatin1(kDefaultLayoutId);
    }
    return layout_id.trimmed();
}

QString MainWindowLayoutGroup(const QString& layout_id) {
    return QStringLiteral("%1/%2/%3")
        .arg(QString::fromLatin1(kGrpMainWindow), QString::fromLatin1(kGrpLayouts),
             NormalizedLayoutId(layout_id));
}

template <typename Value>
QString KeyName(SettingsKey<Value> key) {
    return QString::fromLatin1(key.name);
}

template <typename Value>
QString KeyPath(SettingsKey<Value> key) {
    return QStringLiteral("%1/%2").arg(QString::fromLatin1(key.group),
                                       QString::fromLatin1(key.name));
}

template <typename Value>
QVariant ReadSetting(QSettings& settings, SettingsKey<Value> key, const QVariant& fallback) {
    GroupGuard group(settings, key.group);
    return settings.value(KeyName(key), fallback);
}

template <typename Value>
void WriteSetting(QSettings& settings, SettingsKey<Value> key, const QVariant& value) {
    GroupGuard group(settings, key.group);
    settings.setValue(KeyName(key), value);
}

QString SettingsStatusError(const QSettings& settings, QStringView action) {
    switch (settings.status()) {
        case QSettings::NoError:
            return {};
        case QSettings::AccessError:
            return QStringLiteral("%1 failed: settings access error").arg(action);
        case QSettings::FormatError:
            return QStringLiteral("%1 failed: settings format error").arg(action);
    }
    return QStringLiteral("%1 failed: unknown settings error").arg(action);
}

app::VoidResult<QString> SyncSettings(QSettings& settings, QStringView action) {
    settings.sync();
    const QString error = SettingsStatusError(settings, action);
    if (!error.isEmpty()) {
        return std::unexpected(error);
    }
    return {};
}

void CopySettings(const QSettings& source, QSettings& target) {
    for (const QString& key : source.allKeys()) {
        target.setValue(key, source.value(key));
    }
}

app::VoidResult<QString> EnsureParentDir(const QString& path) {
    const QFileInfo file_info(path);
    const QDir parent_dir = file_info.absoluteDir();
    if (parent_dir.exists()) {
        return {};
    }
    if (parent_dir.mkpath(QStringLiteral("."))) {
        return {};
    }
    return std::unexpected(QStringLiteral("Failed to create settings export directory '%1'")
                               .arg(parent_dir.absolutePath()));
}

std::string ToStdString(const QString& value) {
    return value.toStdString();
}

std::string NormalizedToken(const QString& value) {
    return ToStdString(value.trimmed().toLower());
}

QString ToQString(std::string_view value) {
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

app::logging::LogLevel ReadLogLevel(QSettings& settings, SettingsKey<QString> key,
                                    app::logging::LogLevel fallback) {
    const QString value = ReadSetting(settings, key, ToQString(app::logging::ToString(fallback)))
                              .toString()
                              .trimmed();
    return app::logging::LogLevelFromString(NormalizedToken(value)).value_or(fallback);
}

app::logging::LogSinkType ReadLogSinkType(QSettings& settings, SettingsKey<QString> key,
                                          app::logging::LogSinkType fallback) {
    const QString value = ReadSetting(settings, key, ToQString(app::logging::ToString(fallback)))
                              .toString()
                              .trimmed();
    return app::logging::LogSinkTypeFromString(NormalizedToken(value)).value_or(fallback);
}

SettingsMigrationStep LegacyWorkspaceStateMigration() {
    return {
        .from_version = 0,
        .to_version = 1,
        .description =
            QStringLiteral("Move legacy workspace state into the default layout profile"),
        .migrate = [](QSettings& settings) -> app::VoidResult<QString> {
            const QByteArray legacy_state =
                settings.value(KeyPath(kLegacyWorkspaceStateKey)).toByteArray();
            if (legacy_state.isEmpty()) {
                return {};
            }

            const QString default_layout_key = QStringLiteral("%1/%2").arg(
                MainWindowLayoutGroup(QString::fromLatin1(kDefaultLayoutId)),
                QString::fromLatin1(kKeyLayoutState));
            if (!settings.value(default_layout_key).toByteArray().isEmpty()) {
                return {};
            }

            settings.setValue(default_layout_key, legacy_state);
            return {};
        },
    };
}

std::array<SettingsMigrationStep, 1> BuiltInMigrations() {
    return {LegacyWorkspaceStateMigration()};
}

}  // namespace

WindowSettings::WindowSettings(QSettings& settings) : settings_(settings) {}

void WindowSettings::SaveMainWindowGeometry(const QByteArray& geometry) const {
    WriteSetting(settings_, kMainWindowGeometryKey, geometry);
}

QByteArray WindowSettings::LoadMainWindowGeometry() const {
    return ReadSetting(settings_, kMainWindowGeometryKey, QByteArray()).toByteArray();
}

void WindowSettings::SaveWorkspaceState(const QByteArray& state) const {
    SaveMainWindowLayoutState(QString::fromLatin1(kDefaultLayoutId), state);
}

QByteArray WindowSettings::LoadWorkspaceState() const {
    return LoadMainWindowLayoutState(QString::fromLatin1(kDefaultLayoutId));
}

void WindowSettings::SaveMainWindowLayoutState(const QString& layout_id,
                                               const QByteArray& state) const {
    GroupGuard grp(settings_, MainWindowLayoutGroup(layout_id));
    settings_.setValue(QString::fromLatin1(kKeyLayoutState), state);
}

QByteArray WindowSettings::LoadMainWindowLayoutState(const QString& layout_id) const {
    const QString normalized_layout_id = NormalizedLayoutId(layout_id);

    {
        GroupGuard grp(settings_, MainWindowLayoutGroup(normalized_layout_id));
        const QByteArray state =
            settings_.value(QString::fromLatin1(kKeyLayoutState)).toByteArray();
        if (!state.isEmpty() || normalized_layout_id != QString::fromLatin1(kDefaultLayoutId)) {
            return state;
        }
    }

    GroupGuard grp(settings_, kGrpMainWindow);
    return settings_.value(QString::fromLatin1(kKeyWorkspaceState)).toByteArray();
}

void WindowSettings::SavePageSessionState(const QString& layout_id, const QByteArray& state) const {
    GroupGuard grp(settings_, MainWindowLayoutGroup(layout_id));
    settings_.setValue(QString::fromLatin1(kKeyPageSessionState), state);
}

QByteArray WindowSettings::LoadPageSessionState(const QString& layout_id) const {
    GroupGuard grp(settings_, MainWindowLayoutGroup(layout_id));
    return settings_.value(QString::fromLatin1(kKeyPageSessionState)).toByteArray();
}

void WindowSettings::Reset() const {
    settings_.remove(QString::fromLatin1(kGrpMainWindow));
}

ThemeSettings::ThemeSettings(QSettings& settings) : settings_(settings) {}

bool ThemeSettings::DarkTheme() const {
    return ReadSetting(settings_, kThemeDarkKey, false).toBool();
}

void ThemeSettings::SetDarkTheme(bool enabled) const {
    WriteSetting(settings_, kThemeDarkKey, enabled);
}

void ThemeSettings::Reset() const {
    settings_.remove(QString::fromLatin1(kGrpUI));
}

GeneralSettings::GeneralSettings(QSettings& settings) : settings_(settings) {}

QString GeneralSettings::LanguageCode() const {
    return ReadSetting(settings_, kLanguageKey, QString::fromLatin1(kDefaultLanguageCode))
        .toString()
        .trimmed()
        .toLower();
}

void GeneralSettings::SetLanguageCode(const QString& language_code) const {
    const QString normalized_language_code = language_code.trimmed().toLower();
    WriteSetting(settings_, kLanguageKey,
                 normalized_language_code.isEmpty()
                     ? QString::fromLatin1(kDefaultLanguageCode)
                     : normalized_language_code);
}

void GeneralSettings::Reset() const {
    settings_.remove(QString::fromLatin1(kGrpGeneral));
}

GenericSettings::GenericSettings(QSettings& settings) : settings_(settings) {}

void GenericSettings::SetValue(const QString& dotted_key, const QVariant& value) const {
    settings_.setValue(dotted_key, value);
}

QVariant GenericSettings::Value(const QString& dotted_key, const QVariant& def) const {
    return settings_.value(dotted_key, def);
}

void GenericSettings::RemoveValue(const QString& dotted_key) const {
    settings_.remove(dotted_key);
}

LoggingSettings::LoggingSettings(QSettings& settings) : settings_(settings) {}

app::logging::LoggerConfig LoggingSettings::LoadLoggerConfig(
    const app::logging::LoggerConfig& defaults) const {
    app::logging::LoggerConfig config = defaults;
    config.sink_type = ReadLogSinkType(settings_, kLogSinkTypeKey, defaults.sink_type);
    config.level = ReadLogLevel(settings_, kLogDefaultLevelKey, defaults.level);
    config.terminal_level = ReadLogLevel(settings_, kLogTerminalLevelKey, defaults.terminal_level);
    config.file_level = ReadLogLevel(settings_, kLogFileLevelKey, defaults.file_level);
    config.console_level = ReadLogLevel(settings_, kLogConsoleLevelKey, defaults.console_level);
    config.memory_level = ReadLogLevel(settings_, kLogMemoryLevelKey, defaults.memory_level);
    config.flush_level = ReadLogLevel(settings_, kLogFlushLevelKey, defaults.flush_level);

    config.log_file_path =
        ReadSetting(settings_, kLogFilePathKey, ToQString(defaults.log_file_path))
            .toString()
            .toStdString();
    config.max_file_size_bytes = static_cast<std::uint64_t>(
        ReadSetting(settings_, kLogMaxFileSizeBytesKey,
                    QVariant::fromValue<qulonglong>(defaults.max_file_size_bytes))
            .toULongLong());
    config.max_files = ReadSetting(settings_, kLogMaxFilesKey, defaults.max_files).toUInt();
    config.memory_max_records = static_cast<std::size_t>(
        ReadSetting(settings_, kLogMemoryMaxRecordsKey,
                    QVariant::fromValue<qulonglong>(defaults.memory_max_records))
            .toULongLong());
    config.include_thread_id =
        ReadSetting(settings_, kLogIncludeThreadIdKey, defaults.include_thread_id).toBool();

    return config;
}

void LoggingSettings::SaveLoggerConfig(const app::logging::LoggerConfig& config) const {
    WriteSetting(settings_, kLogSinkTypeKey, ToQString(app::logging::ToString(config.sink_type)));
    WriteSetting(settings_, kLogDefaultLevelKey, ToQString(app::logging::ToString(config.level)));
    WriteSetting(settings_, kLogTerminalLevelKey,
                 ToQString(app::logging::ToString(config.terminal_level)));
    WriteSetting(settings_, kLogFileLevelKey, ToQString(app::logging::ToString(config.file_level)));
    WriteSetting(settings_, kLogConsoleLevelKey,
                 ToQString(app::logging::ToString(config.console_level)));
    WriteSetting(settings_, kLogMemoryLevelKey,
                 ToQString(app::logging::ToString(config.memory_level)));
    WriteSetting(settings_, kLogFlushLevelKey,
                 ToQString(app::logging::ToString(config.flush_level)));
    WriteSetting(settings_, kLogFilePathKey, ToQString(config.log_file_path));
    WriteSetting(settings_, kLogMaxFileSizeBytesKey,
                 QVariant::fromValue<qulonglong>(config.max_file_size_bytes));
    WriteSetting(settings_, kLogMaxFilesKey, config.max_files);
    WriteSetting(settings_, kLogMemoryMaxRecordsKey,
                 QVariant::fromValue<qulonglong>(config.memory_max_records));
    WriteSetting(settings_, kLogIncludeThreadIdKey, config.include_thread_id);
}

void LoggingSettings::Reset() const {
    settings_.remove(QString::fromLatin1(kGrpLogging));
}

AppSettings::AppSettings()
    : window_(settings_),
      general_(settings_),
      theme_(settings_),
      logging_(settings_),
      generic_(settings_) {}

AppSettings::AppSettings(QSettings::Format format, QSettings::Scope scope,
                         const QString& organization, const QString& application)
    : settings_(format, scope, organization, application),
      window_(settings_),
      general_(settings_),
      theme_(settings_),
      logging_(settings_),
      generic_(settings_) {}

AppSettings& AppSettings::Default() {
    static AppSettings settings;
    return settings;
}

WindowSettings& AppSettings::Window() {
    return window_;
}

GeneralSettings& AppSettings::General() {
    return general_;
}

ThemeSettings& AppSettings::Theme() {
    return theme_;
}

LoggingSettings& AppSettings::Logging() {
    return logging_;
}

GenericSettings& AppSettings::Generic() {
    return generic_;
}

int AppSettings::SchemaVersion() const {
    return ReadSetting(const_cast<QSettings&>(settings_), kSchemaVersionKey, 0).toInt();
}

QString AppSettings::FileName() const {
    return settings_.fileName();
}

app::VoidResult<QString> AppSettings::Migrate() {
    const auto migrations = BuiltInMigrations();
    return Migrate(migrations);
}

app::VoidResult<QString> AppSettings::Migrate(std::span<const SettingsMigrationStep> migrations) {
    return Migrate(kCurrentSettingsSchemaVersion, migrations);
}

app::VoidResult<QString> AppSettings::Migrate(int target_schema_version,
                                              std::span<const SettingsMigrationStep> migrations) {
    int version = SchemaVersion();
    if (version > target_schema_version) {
        return std::unexpected(
            QStringLiteral("Settings schema %1 is newer than this app supports (%2)")
                .arg(version)
                .arg(target_schema_version));
    }

    while (version < target_schema_version) {
        const auto migration_it =
            std::ranges::find_if(migrations, [version](const SettingsMigrationStep& migration) {
                return migration.from_version == version;
            });
        if (migration_it == migrations.end()) {
            return std::unexpected(
                QStringLiteral("No settings migration registered from schema %1").arg(version));
        }
        if (!migration_it->migrate) {
            return std::unexpected(
                QStringLiteral("Settings migration from schema %1 has no migration function")
                    .arg(version));
        }

        const app::VoidResult<QString> result = migration_it->migrate(settings_);
        if (!result) {
            return result;
        }

        version = migration_it->to_version;
        SetSchemaVersion(version);
    }

    SetSchemaVersion(target_schema_version);
    return Sync();
}

void AppSettings::ResetToDefaults() {
    settings_.clear();
    SetSchemaVersion(kCurrentSettingsSchemaVersion);
    settings_.sync();
}

app::VoidResult<QString> AppSettings::ExportToIni(const QString& path) const {
    if (path.trimmed().isEmpty()) {
        return std::unexpected(QStringLiteral("Settings export path is empty"));
    }

    const app::VoidResult<QString> parent_dir_result = EnsureParentDir(path);
    if (!parent_dir_result) {
        return parent_dir_result;
    }

    QSettings exported_settings(path, QSettings::IniFormat);
    exported_settings.clear();
    CopySettings(settings_, exported_settings);
    return SyncSettings(exported_settings, QStringLiteral("Settings export"));
}

app::VoidResult<QString> AppSettings::ImportFromIni(const QString& path, SettingsImportMode mode) {
    const QFileInfo file_info(path);
    if (!file_info.exists() || !file_info.isReadable()) {
        return std::unexpected(
            QStringLiteral("Settings import file is not readable: '%1'").arg(path));
    }

    QSettings imported_settings(path, QSettings::IniFormat);
    const QString import_error =
        SettingsStatusError(imported_settings, QStringLiteral("Settings import"));
    if (!import_error.isEmpty()) {
        return std::unexpected(import_error);
    }

    if (mode == SettingsImportMode::kReplace) {
        settings_.clear();
    }
    CopySettings(imported_settings, settings_);
    const app::VoidResult<QString> migration_result = Migrate();
    if (!migration_result) {
        return migration_result;
    }
    return Sync();
}

app::VoidResult<QString> AppSettings::Sync() const {
    return SyncSettings(const_cast<QSettings&>(settings_), QStringLiteral("Settings sync"));
}

void AppSettings::SetSchemaVersion(int version) const {
    WriteSetting(const_cast<QSettings&>(settings_), kSchemaVersionKey, version);
}

}  // namespace app::configs
