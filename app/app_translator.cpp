#include "app/app_translator.h"

#include <QCoreApplication>
#include <QString>

#include "app/app_info.h"
#include "logging/logger.h"

namespace app {
namespace {

constexpr auto kEnglishLanguageCode = "en";
constexpr auto kRussianLanguageCode = "ru";
constexpr auto kTranslatorLogCategory = "translations";

QString NormalizedLanguageCode(const QString& language_code) {
    const QString normalized = language_code.trimmed().toLower();
    return normalized.isEmpty() ? QString::fromLatin1(kEnglishLanguageCode) : normalized;
}

}  // namespace

bool AppTranslator::ApplyLanguage(const QString& language_code, const AppInfo& info) {
    const QString normalized_language_code = NormalizedLanguageCode(language_code);
    QCoreApplication::removeTranslator(&translator_);
    current_language_code_ = QString::fromLatin1(kEnglishLanguageCode);

    if (normalized_language_code == QString::fromLatin1(kEnglishLanguageCode)) {
        return true;
    }

    if (normalized_language_code != QString::fromLatin1(kRussianLanguageCode)) {
        logging::Logger::WarnFmtFor(kTranslatorLogCategory,
                                    "Unsupported language '{}'; falling back to English",
                                    normalized_language_code.toStdString());
        return false;
    }

    const QString translation_path =
        QStringLiteral(":/translations/%1_%2.qm").arg(info.slug, normalized_language_code);
    if (!translator_.load(translation_path)) {
        logging::Logger::WarnFmtFor(kTranslatorLogCategory, "Failed to load translation '{}'",
                                    translation_path.toStdString());
        return false;
    }

    QCoreApplication::installTranslator(&translator_);
    current_language_code_ = normalized_language_code;
    return true;
}

QString AppTranslator::CurrentLanguageCode() const {
    return current_language_code_;
}

}  // namespace app
