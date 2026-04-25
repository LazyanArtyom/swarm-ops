#include "dialogs/message_dialog.h"

#include <QApplication>
#include <QDialogButtonBox>
#include <QGridLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QStyle>
#include <QToolButton>

#include "theme/theme_metrics.h"

namespace app::ui {
namespace {

constexpr int kIconRow = 0;
constexpr int kIconColumn = 0;
constexpr int kMessageRow = 0;
constexpr int kMessageColumn = 1;
constexpr int kDetailsToggleRow = 1;
constexpr int kDetailsRow = 2;
constexpr int kButtonRow = 3;
constexpr int kFullWidthStartColumn = 0;
constexpr int kSingleRowSpan = 1;
constexpr int kFullWidthColumnSpan = 2;

}  // namespace

QIcon MessageDialog::IconFor(Severity severity) {
    auto* style = QApplication::style();
    switch (severity) {
        case Severity::kInfo:
            return style->standardIcon(QStyle::SP_MessageBoxInformation);
        case Severity::kWarning:
            return style->standardIcon(QStyle::SP_MessageBoxWarning);
        case Severity::kError:
            return style->standardIcon(QStyle::SP_MessageBoxCritical);
        case Severity::kConfirm:
            return style->standardIcon(QStyle::SP_MessageBoxQuestion);
    }
    return {};
}

QString MessageDialog::DefaultTitleFor(Severity severity) {
    switch (severity) {
        case Severity::kInfo:
            return QObject::tr("Information");
        case Severity::kWarning:
            return QObject::tr("Warning");
        case Severity::kError:
            return QObject::tr("Error");
        case Severity::kConfirm:
            return QObject::tr("Confirm");
    }
    return {};
}

MessageDialog::MessageDialog(const Options& opts, QWidget* parent) : QDialog(parent) {
    const auto metrics = theme::ThemeMetrics::Instance().Current();
    const int dialog_padding_px = metrics.spacing_md_px;

    setWindowTitle(opts.title.isEmpty() ? DefaultTitleFor(opts.severity) : opts.title);
    setModal(true);

    auto* g_layout = new QGridLayout(this);
    g_layout->setContentsMargins(dialog_padding_px, dialog_padding_px, dialog_padding_px,
                                 dialog_padding_px);
    g_layout->setHorizontalSpacing(dialog_padding_px);
    g_layout->setVerticalSpacing(dialog_padding_px);

    auto* icon_lbl = new QLabel(this);
    icon_lbl->setPixmap(
        IconFor(opts.severity).pixmap(metrics.dialog_icon_px, metrics.dialog_icon_px));
    icon_lbl->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    g_layout->addWidget(icon_lbl, kIconRow, kIconColumn);

    auto* msg_lbl = new QLabel(this);
    msg_lbl->setTextFormat(Qt::RichText);
    msg_lbl->setTextInteractionFlags(Qt::TextBrowserInteraction | Qt::LinksAccessibleByMouse);
    msg_lbl->setOpenExternalLinks(true);
    msg_lbl->setWordWrap(true);
    msg_lbl->setMinimumWidth(metrics.dialog_message_min_width_px);
    msg_lbl->setText(opts.message_html);
    g_layout->addWidget(msg_lbl, kMessageRow, kMessageColumn);

    QPlainTextEdit* details = nullptr;
    QToolButton* toggle_details_btn = nullptr;
    if (!opts.details_text.isEmpty()) {
        toggle_details_btn = new QToolButton(this);
        toggle_details_btn->setText(tr("Details…"));
        toggle_details_btn->setCheckable(true);
        g_layout->addWidget(toggle_details_btn, kDetailsToggleRow, kMessageColumn, Qt::AlignLeft);

        details = new QPlainTextEdit(this);
        details->setReadOnly(true);
        details->setPlainText(opts.details_text);
        details->setMinimumHeight(metrics.dialog_details_min_height_px);
        details->setVisible(opts.show_details_by_default);
        g_layout->addWidget(details, kDetailsRow, kFullWidthStartColumn, kSingleRowSpan,
                            kFullWidthColumnSpan);

        toggle_details_btn->setChecked(opts.show_details_by_default);
        connect(toggle_details_btn, &QToolButton::toggled, details, &QPlainTextEdit::setVisible);
    }

    auto* btns = new QDialogButtonBox(opts.buttons, this);
    QPushButton* def_btn = btns->button(opts.default_button);
    if (def_btn != nullptr) {
        def_btn->setDefault(true);
    }
    g_layout->addWidget(btns, kButtonRow, kFullWidthStartColumn, kSingleRowSpan,
                        kFullWidthColumnSpan, Qt::AlignRight);

    // Connection
    connect(btns, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(btns, &QDialogButtonBox::rejected, this, &QDialog::reject);
}

QDialog::DialogCode MessageDialog::ShowInfo(QWidget* parent, const QString& title,
                                            const QString& message_html,
                                            const QString& details_text) {
    Options options;
    options.severity = Severity::kInfo;
    options.title = title;
    options.message_html = message_html;
    options.details_text = details_text;
    options.buttons = QDialogButtonBox::Ok;
    options.default_button = QDialogButtonBox::Ok;
    MessageDialog dlg(options, parent);
    return static_cast<QDialog::DialogCode>(dlg.exec());
}

QDialog::DialogCode MessageDialog::ShowWarning(QWidget* parent, const QString& title,
                                               const QString& message_html,
                                               const QString& details_text) {
    Options options;
    options.severity = Severity::kWarning;
    options.title = title;
    options.message_html = message_html;
    options.details_text = details_text;
    options.buttons = QDialogButtonBox::Ok;
    options.default_button = QDialogButtonBox::Ok;
    MessageDialog dlg(options, parent);
    return static_cast<QDialog::DialogCode>(dlg.exec());
}

QDialog::DialogCode MessageDialog::ShowError(QWidget* parent, const QString& title,
                                             const QString& message_html,
                                             const QString& details_text) {
    Options options;
    options.severity = Severity::kError;
    options.title = title;
    options.message_html = message_html;
    options.details_text = details_text;
    options.buttons = QDialogButtonBox::Ok;
    options.default_button = QDialogButtonBox::Ok;
    MessageDialog dlg(options, parent);
    return static_cast<QDialog::DialogCode>(dlg.exec());
}

QDialog::DialogCode MessageDialog::Confirm(QWidget* parent, const QString& title,
                                           const QString& message_html,
                                           const QString& details_text) {
    Options options;
    options.severity = Severity::kConfirm;
    options.title = title;
    options.message_html = message_html;
    options.details_text = details_text;
    options.buttons = QDialogButtonBox::Yes | QDialogButtonBox::No;
    options.default_button = QDialogButtonBox::No;
    MessageDialog dlg(options, parent);
    return static_cast<QDialog::DialogCode>(dlg.exec());
}

}  // namespace app::ui
