#pragma once

#include <QTranslator>

class QString;

namespace app {

struct AppInfo;

class AppTranslator final {
   public:
    AppTranslator() = default;
    ~AppTranslator() = default;

    [[nodiscard]] bool ApplyLanguage(const QString& language_code, const AppInfo& info);
    [[nodiscard]] QString CurrentLanguageCode() const;

   private:
    QTranslator translator_;
    QString current_language_code_;
};

}  // namespace app
