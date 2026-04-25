#include "panels/central_panel.h"

#include <QStackedWidget>
#include <QTabWidget>
#include <QtTest/QtTest>

#include "workspace/page_lifecycle.h"
#include "workspace/page_registry.h"

namespace {

constexpr auto kPageKey = "page.test";
constexpr auto kFirstPageKey = "page.first";
constexpr auto kSecondPageKey = "page.second";
constexpr auto kLockedPageKey = "page.locked";
constexpr auto kClosablePageKey = "page.closable";

struct PanelHosts final {
    QStackedWidget* stacked{nullptr};
    QTabWidget* tabs{nullptr};
};

class LifecyclePage final : public QWidget, public app::ui::workspace::IPageLifecycle {
   public:
    explicit LifecyclePage(QWidget* parent = nullptr) : QWidget(parent) {}

    void OnActivate() override {
        ++activate_count;
    }

    void OnDeactivate() override {
        ++deactivate_count;
    }

    [[nodiscard]] bool CanClose() const override {
        return can_close;
    }

    int activate_count{0};
    int deactivate_count{0};
    bool can_close{true};
};

QWidget* NewTestPage(QWidget* parent) {
    return new QWidget(parent);
}

PanelHosts FindHosts(app::ui::CentralPanel* panel) {
    if (panel == nullptr) {
        return {};
    }

    return {
        .stacked = panel->findChild<QStackedWidget*>(),
        .tabs = panel->findChild<QTabWidget*>(),
    };
}

void VerifySingleHost(const PanelHosts& hosts, QWidget* page) {
    QVERIFY(hosts.stacked != nullptr);
    QVERIFY(hosts.tabs != nullptr);
    QVERIFY(hosts.stacked->indexOf(page) >= 0);
    QCOMPARE(hosts.tabs->indexOf(page), -1);
}

void VerifyTabHost(const PanelHosts& hosts, QWidget* page) {
    QVERIFY(hosts.stacked != nullptr);
    QVERIFY(hosts.tabs != nullptr);
    QCOMPARE(hosts.stacked->indexOf(page), -1);
    QVERIFY(hosts.tabs->indexOf(page) >= 0);
}

}  // namespace

class CentralPanelTest : public QObject {
    Q_OBJECT

   private slots:
    static void ShowsRegisteredPageInSingleMode() {
        app::ui::CentralPanel panel;
        panel.RegisterPageFactory(QString::fromLatin1(kPageKey), NewTestPage);

        QWidget* page = panel.ShowPage(QString::fromLatin1(kPageKey));

        QVERIFY(page != nullptr);
        QCOMPARE(panel.CurrentMode(), app::ui::CentralPanel::Mode::kSingle);
        QCOMPARE(panel.CurrentPageKey(), QString::fromLatin1(kPageKey));
    }

    static void ReparentsPageBetweenSingleAndTabHosts() {
        app::ui::CentralPanel panel;
        panel.RegisterPageFactory(QString::fromLatin1(kPageKey), NewTestPage);

        const PanelHosts hosts = FindHosts(&panel);

        QWidget* single_page = panel.ShowPage(QString::fromLatin1(kPageKey));
        QVERIFY(single_page != nullptr);
        VerifySingleHost(hosts, single_page);

        panel.SetMode(app::ui::CentralPanel::Mode::kTabs);
        QWidget* tab_page = panel.ShowPage(QString::fromLatin1(kPageKey));
        QCOMPARE(tab_page, single_page);
        VerifyTabHost(hosts, single_page);

        panel.SetMode(app::ui::CentralPanel::Mode::kSingle);
        QWidget* restored_page = panel.ShowPage(QString::fromLatin1(kPageKey));
        QCOMPARE(restored_page, single_page);
        VerifySingleHost(hosts, single_page);
    }

    static void RunsLifecycleHooksWhenCurrentPageChanges() {
        app::ui::CentralPanel panel;
        LifecyclePage* first_page = nullptr;
        LifecyclePage* second_page = nullptr;

        panel.SetPageRegistry(nullptr);
        panel.RegisterPageFactory(QString::fromLatin1(kFirstPageKey),
                                  [&first_page](QWidget* parent) {
                                      first_page = new LifecyclePage(parent);
                                      return first_page;
                                  });
        panel.RegisterPageFactory(QString::fromLatin1(kSecondPageKey),
                                  [&second_page](QWidget* parent) {
                                      second_page = new LifecyclePage(parent);
                                      return second_page;
                                  });

        QVERIFY(panel.ShowPage(QString::fromLatin1(kFirstPageKey)) != nullptr);
        QVERIFY(panel.ShowPage(QString::fromLatin1(kSecondPageKey)) != nullptr);

        QVERIFY(first_page != nullptr);
        QVERIFY(second_page != nullptr);
        QCOMPARE(first_page->activate_count, 1);
        QCOMPARE(first_page->deactivate_count, 1);
        QCOMPARE(second_page->activate_count, 1);
        QCOMPARE(second_page->deactivate_count, 0);
    }

    static void HonorsClosablePageDescriptorInTabs() {
        app::ui::CentralPanel panel;
        app::ui::workspace::PageRegistry registry;
        panel.SetPageRegistry(&registry);
        panel.SetMode(app::ui::CentralPanel::Mode::kTabs);

        registry.RegisterPage({
            .page_key = QString::fromLatin1(kLockedPageKey),
            .title = QStringLiteral("Locked"),
            .factory = [](QWidget* parent) { return new LifecyclePage(parent); },
            .default_open_mode = app::ui::workspace::PageOpenMode::kTab,
            .closable = false,
        });
        registry.RegisterPage({
            .page_key = QString::fromLatin1(kClosablePageKey),
            .title = QStringLiteral("Closable"),
            .factory = [](QWidget* parent) { return new LifecyclePage(parent); },
            .default_open_mode = app::ui::workspace::PageOpenMode::kTab,
            .closable = true,
        });

        const PanelHosts hosts = FindHosts(&panel);
        QVERIFY(panel.ShowPage(QString::fromLatin1(kLockedPageKey)) != nullptr);
        QCOMPARE(hosts.tabs->count(), 1);
        QMetaObject::invokeMethod(hosts.tabs, "tabCloseRequested", Qt::DirectConnection,
                                  Q_ARG(int, 0));
        QCOMPARE(hosts.tabs->count(), 1);

        QVERIFY(panel.ShowPage(QString::fromLatin1(kClosablePageKey)) != nullptr);
        QCOMPARE(hosts.tabs->count(), 2);
        QMetaObject::invokeMethod(hosts.tabs, "tabCloseRequested", Qt::DirectConnection,
                                  Q_ARG(int, 1));
        QCOMPARE(hosts.tabs->count(), 1);
    }
};

QTEST_MAIN(CentralPanelTest)

#include "central_panel_test.moc"
