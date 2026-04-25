#include "configs/app_configs.h"

#include <QByteArray>
#include <QDir>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest/QtTest>
#include <array>

namespace {

constexpr int kStoredAnswer = 42;
constexpr int kFallbackAnswer = 7;
constexpr std::size_t kStoredMemoryRecordLimit = 256U;
constexpr auto kSettingsTestDir = "/tmp/app_core_tests_settings";
constexpr auto kExportFileName = "settings-export.ini";
constexpr int kProjectSettingsSchemaVersion = 2;

void ClearSettings() {
    QSettings settings;
    settings.clear();
    settings.sync();
}

bool LoggerConfigsMatch(const app::logging::LoggerConfig& left,
                        const app::logging::LoggerConfig& right) {
    return left.sink_type == right.sink_type && left.level == right.level &&
           left.terminal_level == right.terminal_level && left.file_level == right.file_level &&
           left.console_level == right.console_level && left.memory_level == right.memory_level &&
           left.flush_level == right.flush_level && left.log_file_path == right.log_file_path &&
           left.max_file_size_bytes == right.max_file_size_bytes &&
           left.max_files == right.max_files &&
           left.memory_max_records == right.memory_max_records &&
           left.include_thread_id == right.include_thread_id;
}

}  // namespace

class AppConfigsTest : public QObject {
    Q_OBJECT

   private slots:
    static void initTestCase() {
        QCoreApplication::setOrganizationName("SwarmOpsTests");
        QCoreApplication::setApplicationName("app_core_tests");
        QStandardPaths::setTestModeEnabled(true);
        QSettings::setDefaultFormat(QSettings::IniFormat);
        QDir(kSettingsTestDir).removeRecursively();
        QDir().mkpath(QString::fromLatin1(kSettingsTestDir));
        QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
                           QString::fromLatin1(kSettingsTestDir));
    }

    static void cleanupTestCase() {
        QDir(kSettingsTestDir).removeRecursively();
    }

    static void init() {
        ClearSettings();
    }

    static void SavesAndLoadsMainWindowGeometry() {
        auto& settings = app::configs::AppSettings::Default();
        const QByteArray geometry("window-geometry");
        settings.Window().SaveMainWindowGeometry(geometry);

        QCOMPARE(settings.Window().LoadMainWindowGeometry(), geometry);
    }

    static void SavesAndLoadsWorkspaceState() {
        auto& settings = app::configs::AppSettings::Default();
        const QByteArray state("workspace-state");
        settings.Window().SaveWorkspaceState(state);

        QCOMPARE(settings.Window().LoadWorkspaceState(), state);
    }

    static void SavesAndLoadsNamedMainWindowLayoutState() {
        auto& settings = app::configs::AppSettings::Default();
        const QByteArray default_state("default-layout-state");
        const QByteArray compact_state("compact-layout-state");

        settings.Window().SaveMainWindowLayoutState("default", default_state);
        settings.Window().SaveMainWindowLayoutState("compact", compact_state);

        QCOMPARE(settings.Window().LoadMainWindowLayoutState("default"), default_state);
        QCOMPARE(settings.Window().LoadMainWindowLayoutState("compact"), compact_state);
    }

    static void EmptyLayoutNameUsesDefaultMainWindowLayoutState() {
        auto& settings = app::configs::AppSettings::Default();
        const QByteArray state("default-layout-state");

        settings.Window().SaveMainWindowLayoutState("default", state);

        QCOMPARE(settings.Window().LoadMainWindowLayoutState(QString()), state);
    }

    static void DefaultMainWindowLayoutStateFallsBackToLegacyWorkspaceState() {
        auto& settings = app::configs::AppSettings::Default();
        const QByteArray state("legacy-workspace-state");

        settings.Generic().SetValue("mainwindow/workspace_state", state);

        QCOMPARE(settings.Window().LoadMainWindowLayoutState("default"), state);
    }

    static void StoresAndLoadsGenericValues() {
        auto& settings = app::configs::AppSettings::Default();
        settings.Generic().SetValue("tests.answer", kStoredAnswer);

        QCOMPARE(settings.Generic().Value("tests.answer").toInt(), kStoredAnswer);
        QCOMPARE(settings.Generic().Value("tests.missing", kFallbackAnswer).toInt(),
                 kFallbackAnswer);
    }

    static void SavesAndLoadsLoggingConfig() {
        auto& settings = app::configs::AppSettings::Default();

        app::logging::LoggerConfig config;
        config.sink_type = app::logging::LogSinkType::kTerminalAndRotatingFile;
        config.level = app::logging::LogLevel::kDebug;
        config.terminal_level = app::logging::LogLevel::kWarn;
        config.file_level = app::logging::LogLevel::kTrace;
        config.console_level = app::logging::LogLevel::kError;
        config.memory_level = app::logging::LogLevel::kDebug;
        config.flush_level = app::logging::LogLevel::kError;
        config.log_file_path = "/tmp/app.log";
        config.max_file_size_bytes = app::logging::kBytesPerMebibyte;
        config.max_files = 4U;
        config.memory_max_records = kStoredMemoryRecordLimit;
        config.include_thread_id = false;

        settings.Logging().SaveLoggerConfig(config);

        const app::logging::LoggerConfig loaded =
            settings.Logging().LoadLoggerConfig(app::logging::LoggerConfig{});

        QVERIFY(LoggerConfigsMatch(loaded, config));
    }

    static void SettingsDomainsShareOneBackingStore() {
        auto& settings = app::configs::AppSettings::Default();
        const QByteArray geometry("typed-window-geometry");
        const QByteArray layout_state("typed-layout-state");

        settings.Window().SaveMainWindowGeometry(geometry);
        settings.Window().SaveMainWindowLayoutState("default", layout_state);
        settings.General().SetLanguageCode("ru");
        settings.Theme().SetDarkTheme(true);
        settings.Logging().SaveLoggerConfig(app::logging::LoggerConfig{});
        settings.Generic().SetValue("tests.typed_answer", kStoredAnswer);

        QCOMPARE(settings.Window().LoadMainWindowGeometry(), geometry);
        QCOMPARE(settings.Window().LoadMainWindowLayoutState("default"), layout_state);
        QCOMPARE(settings.General().LanguageCode(), QStringLiteral("ru"));
        QCOMPARE(settings.Theme().DarkTheme(), true);
        QCOMPARE(settings.Logging().LoadLoggerConfig(app::logging::LoggerConfig{}).logger_name,
                 std::string{"app"});
        QCOMPARE(settings.Generic().Value("tests.typed_answer").toInt(), kStoredAnswer);
    }

    static void MigratesLegacyWorkspaceStateToDefaultLayout() {
        auto& settings = app::configs::AppSettings::Default();
        const QByteArray legacy_state("legacy-workspace-state");

        settings.Generic().SetValue("mainwindow/workspace_state", legacy_state);

        const app::VoidResult<QString> result = settings.Migrate();

        QVERIFY(result.has_value());
        QCOMPARE(settings.SchemaVersion(), app::configs::kCurrentSettingsSchemaVersion);
        QCOMPARE(settings.Window().LoadMainWindowLayoutState("default"), legacy_state);
    }

    static void ResetToDefaultsClearsValuesAndKeepsSchemaVersion() {
        auto& settings = app::configs::AppSettings::Default();

        settings.Theme().SetDarkTheme(true);
        settings.General().SetLanguageCode("ru");
        settings.Generic().SetValue("tests.answer", kStoredAnswer);
        settings.ResetToDefaults();

        QCOMPARE(settings.SchemaVersion(), app::configs::kCurrentSettingsSchemaVersion);
        QCOMPARE(settings.General().LanguageCode(), QStringLiteral("en"));
        QCOMPARE(settings.Theme().DarkTheme(), false);
        QVERIFY(!settings.Generic().Value("tests.answer").isValid());
    }

    static void ResetsIndividualSettingsDomains() {
        auto& settings = app::configs::AppSettings::Default();
        const QByteArray geometry("window-geometry");

        settings.Window().SaveMainWindowGeometry(geometry);
        settings.General().SetLanguageCode("ru");
        settings.Theme().SetDarkTheme(true);
        settings.Window().Reset();

        QVERIFY(settings.Window().LoadMainWindowGeometry().isEmpty());
        QCOMPARE(settings.General().LanguageCode(), QStringLiteral("ru"));
        QCOMPARE(settings.Theme().DarkTheme(), true);
    }

    static void ExportsAndImportsSettingsIni() {
        auto& settings = app::configs::AppSettings::Default();
        QTemporaryDir export_dir;
        QVERIFY(export_dir.isValid());
        const QString export_path = export_dir.filePath(QString::fromLatin1(kExportFileName));

        settings.Theme().SetDarkTheme(true);
        settings.Generic().SetValue("tests.answer", kStoredAnswer);

        const app::VoidResult<QString> export_result = settings.ExportToIni(export_path);
        QVERIFY(export_result.has_value());

        settings.ResetToDefaults();
        QCOMPARE(settings.Theme().DarkTheme(), false);

        const app::VoidResult<QString> import_result =
            settings.ImportFromIni(export_path, app::configs::SettingsImportMode::kReplace);
        QVERIFY(import_result.has_value());

        QCOMPARE(settings.SchemaVersion(), app::configs::kCurrentSettingsSchemaVersion);
        QCOMPARE(settings.Theme().DarkTheme(), true);
        QCOMPARE(settings.Generic().Value("tests.answer").toInt(), kStoredAnswer);
    }

    static void RunsProjectMigrationHooks() {
        auto& settings = app::configs::AppSettings::Default();
        const std::array migrations{
            app::configs::SettingsMigrationStep{
                .from_version = 0,
                .to_version = app::configs::kCurrentSettingsSchemaVersion,
                .description = QStringLiteral("Core migration"),
                .migrate = [](QSettings&) -> app::VoidResult<QString> { return {}; },
            },
            app::configs::SettingsMigrationStep{
                .from_version = app::configs::kCurrentSettingsSchemaVersion,
                .to_version = kProjectSettingsSchemaVersion,
                .description = QStringLiteral("Project migration"),
                .migrate =
                    [](QSettings& migrated_settings) -> app::VoidResult<QString> {
                    migrated_settings.setValue(QStringLiteral("tests/migrated"), true);
                    return {};
                },
            },
        };

        const app::VoidResult<QString> result =
            settings.Migrate(kProjectSettingsSchemaVersion, migrations);

        QVERIFY(result.has_value());
        QCOMPARE(settings.SchemaVersion(), kProjectSettingsSchemaVersion);
        QCOMPARE(settings.Generic().Value("tests/migrated").toBool(), true);
    }
};

QTEST_APPLESS_MAIN(AppConfigsTest)

#include "app_configs_test.moc"
