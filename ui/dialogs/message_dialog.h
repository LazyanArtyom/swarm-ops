#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QIcon>
#include <QString>
#include <cstdint>

namespace app::ui {

class MessageDialog final : public QDialog {
    Q_OBJECT
   public:
    enum class Severity : std::uint8_t { kInfo, kWarning, kError, kConfirm };

    struct Options {
        Severity severity = Severity::kInfo;
        QString title;
        QString message_html;
        QString details_text;
        QDialogButtonBox::StandardButtons buttons = QDialogButtonBox::Ok;
        QDialogButtonBox::StandardButton default_button = QDialogButtonBox::Ok;
        bool show_details_by_default = false;
    };

    explicit MessageDialog(const Options& opts, QWidget* parent = nullptr);
    ~MessageDialog() override = default;

    static QDialog::DialogCode ShowInfo(QWidget* parent, const QString& title,
                                        const QString& message_html,
                                        const QString& details_text = QString());

    static QDialog::DialogCode ShowWarning(QWidget* parent, const QString& title,
                                           const QString& message_html,
                                           const QString& details_text = QString());

    static QDialog::DialogCode ShowError(QWidget* parent, const QString& title,
                                         const QString& message_html,
                                         const QString& details_text = QString());

    static QDialog::DialogCode Confirm(QWidget* parent, const QString& title,
                                       const QString& message_html,
                                       const QString& details_text = QString());

   private:
    static QIcon IconFor(Severity severity);
    static QString DefaultTitleFor(Severity severity);
};

}  // namespace app::ui
