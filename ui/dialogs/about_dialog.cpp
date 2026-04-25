#include "dialogs/about_dialog.h"

#include <QCoreApplication>
#include <QDialogButtonBox>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSysInfo>
#include <QVBoxLayout>

#include "object_names.h"
#include "theme/theme_metrics.h"

namespace app::ui {
namespace {

constexpr int kNoStretch = 0;

}  // namespace

AboutDialog::AboutDialog(QWidget* parent) : QDialog(parent) {
    setObjectName(QString::fromLatin1(object_names::kAboutDialog));
    setWindowTitle(tr("About"));
    setModal(true);
    setMinimumWidth(theme::ThemeMetrics::Instance().Current().dialog_message_min_width_px);

    const QString app_name = QCoreApplication::applicationName();
    const QString app_version = QCoreApplication::applicationVersion();
    const QString qt_version = QString::fromLatin1(QT_VERSION_STR);
    const QString product_name = QSysInfo::prettyProductName();
    const QString built_on =
        QStringLiteral("%1 %2").arg(QString::fromLatin1(__DATE__), QString::fromLatin1(__TIME__));

    auto* card = new QWidget(this);
    auto* v_layout_card = new QVBoxLayout(card);

    auto* title = new QLabel(QStringLiteral("%1 %2").arg(app_name, app_version), card);
    title->setTextFormat(Qt::PlainText);
    v_layout_card->addWidget(title);

    auto* meta = new QLabel(card);
    meta->setText(tr("Qt %1\n%2").arg(qt_version, product_name));
    meta->setTextFormat(Qt::PlainText);
    v_layout_card->addWidget(meta);

    auto* footer = new QLabel(tr("© Company name\nBuilt on %1").arg(built_on), card);
    footer->setTextFormat(Qt::PlainText);
    v_layout_card->addWidget(footer);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    auto* v_layout_root = new QVBoxLayout(this);
    v_layout_root->addWidget(card);
    v_layout_root->addWidget(buttons, kNoStretch, Qt::AlignRight);

    // Connections
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
}

}  // namespace app::ui
