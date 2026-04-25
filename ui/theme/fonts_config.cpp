#include "theme/fonts_config.h"

#include <QFontDatabase>
#include <QStringList>
#include <array>
#include <string_view>

#include "logging/logger.h"

namespace app::ui::theme::fonts {
namespace {

constexpr std::string_view kThemeLogCategory = "theme";

constexpr std::array kUiFontResources{
    std::string_view{":/theme/fonts/Inter-Regular.ttf"},
    std::string_view{":/theme/fonts/Inter-Medium.ttf"},
    std::string_view{":/theme/fonts/Inter-Italic.ttf"},
    std::string_view{":/theme/fonts/Inter-SemiBold.ttf"},
    std::string_view{":/theme/fonts/InterVariable.ttf"},
    std::string_view{":/theme/fonts/InterVariable-Italic.ttf"},
};

constexpr std::array kMonoFontResources{
    std::string_view{":/theme/fonts/JetBrainsMono-Regular.ttf"},
    std::string_view{":/theme/fonts/JetBrainsMono-Medium.ttf"},
    std::string_view{":/theme/fonts/JetBrainsMono-Italic.ttf"},
    std::string_view{":/theme/fonts/JetBrainsMono-SemiBold.ttf"},
    std::string_view{":/theme/fonts/JetBrainsMono[wght].ttf"},
    std::string_view{":/theme/fonts/JetBrainsMono-Italic[wght].ttf"},
};

struct FontState final {
    QString ui_family;
    QString mono_family;
    bool initialized{false};
};

FontState& State() {
    static FontState state;
    return state;
}

QString ToQString(std::string_view value) {
    return QString::fromUtf8(value.data(), static_cast<qsizetype>(value.size()));
}

std::string ToStdString(const QString& value) {
    return value.toStdString();
}

bool AddFont(std::string_view resource_path) {
    const QString qrc_path = ToQString(resource_path);
    const int font_id = QFontDatabase::addApplicationFont(qrc_path);
    if (font_id < 0) {
        app::logging::Logger::WarnFmtFor(kThemeLogCategory, "Font resource could not be loaded: {}",
                                         resource_path);
        return false;
    }
    return true;
}

QString PickFamily(const QStringList& candidates) {
    const QStringList families = QFontDatabase::families();
    for (const QString& want : candidates) {
        if (families.contains(want)) {
            return want;
        }
        for (const QString& family : families) {
            if (family.compare(want, Qt::CaseInsensitive) == 0) {
                return family;
            }
            if (family.startsWith(want, Qt::CaseInsensitive)) {
                return family;
            }
        }
    }
    return {};
}

QStringList UiFontCandidates() {
    return {
        QStringLiteral("Inter"),
        QStringLiteral("Inter UI"),
        QStringLiteral("SF Pro Text"),
        QStringLiteral("San Francisco"),
        QStringLiteral("Segoe UI"),
        QStringLiteral("Noto Sans"),
        QStringLiteral("Ubuntu"),
        QStringLiteral("Cantarell"),
    };
}

QStringList MonoFontCandidates() {
    return {
        QStringLiteral("JetBrains Mono"),
        QStringLiteral("SF Mono"),
        QStringLiteral("Menlo"),
        QStringLiteral("Monaco"),
        QStringLiteral("Fira Code"),
    };
}

QString SystemUiFamily() {
    return QFontDatabase::systemFont(QFontDatabase::GeneralFont).family();
}

QString SystemMonoFamily() {
    return QFontDatabase::systemFont(QFontDatabase::FixedFont).family();
}

void LogFallbackFont(std::string_view role, const QString& family) {
    app::logging::Logger::WarnFmtFor(kThemeLogCategory,
                                     "Preferred {} font families were not found; using '{}'", role,
                                     ToStdString(family));
}

}  // namespace

void Init() {
    FontState& state = State();
    if (state.initialized) {
        return;
    }
    state.initialized = true;

    for (const std::string_view resource_path : kUiFontResources) {
        (void)AddFont(resource_path);
    }

    for (const std::string_view resource_path : kMonoFontResources) {
        (void)AddFont(resource_path);
    }

    state.ui_family = PickFamily(UiFontCandidates());
    state.mono_family = PickFamily(MonoFontCandidates());

    if (state.ui_family.isEmpty()) {
        state.ui_family = SystemUiFamily();
        LogFallbackFont("UI", state.ui_family);
    }
    if (state.mono_family.isEmpty()) {
        state.mono_family = SystemMonoFamily();
        LogFallbackFont("monospace", state.mono_family);
    }
}

QString UiFamily() {
    Init();
    return State().ui_family;
}

QString MonoFamily() {
    Init();
    return State().mono_family;
}

}  // namespace app::ui::theme::fonts
