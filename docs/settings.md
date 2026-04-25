# Application Settings

The client foundation keeps persistent configuration behind `app::configs::AppSettings`.
Application code should normally access it through `AppServices`:

```cpp
const bool dark_theme = context.Services().Settings().Theme().DarkTheme();
context.Services().Settings().Theme().SetDarkTheme(true);
```

For tests or embedded tools, inject a settings instance into `AppContext`:

```cpp
app::configs::AppSettings settings(
    QSettings::IniFormat,
    QSettings::UserScope,
    "SwarmOpsTests",
    "SwarmOpsClientTests");

app::AppContext context(
    app::BuildAppInfo(),
    {.settings = &settings});
```

## Domains

Settings are split into small domain APIs:

- `Window()`: main window geometry and layout profile state.
- `Theme()`: theme preferences.
- `Logging()`: persisted logger sink/level/file settings.
- `Generic()`: escape hatch for project-specific keys.

Prefer domain APIs over raw string keys. Use `Generic()` only for app-specific
settings that do not yet deserve their own domain class.

New reusable features must add a typed settings domain before using
`GenericSettings`. Treat `Generic()` as a last-mile escape hatch, not the
default storage API for client foundation features.

## Schema And Migrations

`AppSettings::Migrate()` upgrades persisted settings to
`kCurrentSettingsSchemaVersion`. `AppBootstrap` runs built-in migrations during
`AppContext` construction, before logging and UI setup use settings.

SwarmOps modules can register their own migration steps:

```cpp
const std::array migrations{
    app::configs::SettingsMigrationStep{
        .from_version = 1,
        .to_version = 2,
        .description = "Move project paths into workspace settings",
        .migrate = [](QSettings& settings) -> app::VoidResult<QString> {
            settings.setValue("workspace/project_root",
                              settings.value("project/root"));
            settings.remove("project/root");
            return {};
        },
    },
};

const auto result = context.Services().Settings().Migrate(2, migrations);
```

## Reset

Reset the whole app:

```cpp
context.Services().Settings().ResetToDefaults();
```

Reset one domain:

```cpp
context.Services().Settings().Window().Reset();
context.Services().Settings().Logging().Reset();
```

## Import And Export

Settings import/export uses INI files so `QSettings` value types such as
`QByteArray` are preserved.

```cpp
const auto export_result =
    context.Services().Settings().ExportToIni("/tmp/swarmops-settings.ini");

const auto import_result = context.Services().Settings().ImportFromIni(
    "/tmp/swarmops-settings.ini",
    app::configs::SettingsImportMode::kReplace);
```

Use `kMerge` when importing only a partial preference file; use `kReplace` when
restoring a full backup.
