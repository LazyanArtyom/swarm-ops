#include <QColor>
#include <QStringConverter>
#include <QTemporaryFile>
#include <QTextStream>
#include <QtTest/QtTest>

#include "theme/theme_colors.h"
#include "theme/theme_loader.h"

namespace {

QString ColorName(const QColor& color) {
    return color.name(QColor::HexArgb).toUpper();
}

void WriteUtf8(QIODevice* device, const QString& text) {
    QTextStream stream(device);
    stream.setEncoding(QStringConverter::Utf8);
    stream << text;
}

}  // namespace

class ThemeSystemTest final : public QObject {
    Q_OBJECT

   private slots:
    static void DarkPaletteMatchesDocumentedDesignSystem() {
        const app::ui::theme::ThemeColors colors = app::ui::theme::ThemeColors::Dark();

        QCOMPARE(ColorName(colors.fg), QStringLiteral("#FFCCCCCC"));
        QCOMPARE(ColorName(colors.fg_muted), QStringLiteral("#FF8C8C8C"));
        QCOMPARE(ColorName(colors.bg_window), QStringLiteral("#FF1F2125"));
        QCOMPARE(ColorName(colors.bg_surface), QStringLiteral("#FF24262A"));
        QCOMPARE(ColorName(colors.border), QStringLiteral("#FF2B2B2B"));
        QCOMPARE(ColorName(colors.primary), QStringLiteral("#FF4285F4"));
    }

    static void LightPaletteMatchesDocumentedDesignSystem() {
        const app::ui::theme::ThemeColors colors = app::ui::theme::ThemeColors::Light();

        QCOMPARE(ColorName(colors.fg), QStringLiteral("#FF202124"));
        QCOMPARE(ColorName(colors.fg_muted), QStringLiteral("#FF5F6368"));
        QCOMPARE(ColorName(colors.bg_window), QStringLiteral("#FFF3F4F6"));
        QCOMPARE(ColorName(colors.bg_surface), QStringLiteral("#FFFFFFFF"));
        QCOMPARE(ColorName(colors.border), QStringLiteral("#FFE0E3E7"));
        QCOMPARE(ColorName(colors.primary), QStringLiteral("#FF1976D2"));
    }

    static void ThemeLoaderReportsUnresolvedTokens() {
        QTemporaryFile qss_file;
        QVERIFY(qss_file.open());
        WriteUtf8(&qss_file, QStringLiteral("QWidget { color: ${MISSING_TOKEN}; }"));
        qss_file.close();

        const app::ui::theme::ThemeLoader::QssResult result =
            app::ui::theme::ThemeLoader::TryLoadQss({qss_file.fileName()}, {});

        QVERIFY(!result);
        QVERIFY(result.error().contains(QStringLiteral("${MISSING_TOKEN}")));
    }
};

QTEST_MAIN(ThemeSystemTest)

#include "theme_system_test.moc"
