#pragma once

#include <QByteArray>
#include <QHash>
#include <QIcon>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>
#include <cstdint>

class QStackedWidget;
class QTabWidget;
class QWidget;

namespace app::ui::workspace {

class PageRegistry;

enum class PageHostMode : std::uint8_t {
    kSingle,
    kTabs,
};

class PageHost final : public QObject {
    Q_OBJECT

   public:
    struct SessionTabEntry final {
        QString page_key;
        QString title;
        QByteArray state;
    };

    struct SessionState final {
        PageHostMode mode{PageHostMode::kSingle};
        QString current_page_key;
        QByteArray current_page_state;
        QList<SessionTabEntry> tabs;
        int current_tab_index{-1};
    };

    struct TabOptions final {
        QString title;
        QIcon icon;
        bool single_instance{true};
        bool closable{true};
    };

    PageHost(QStackedWidget* stacked, QTabWidget* tabs, QObject* parent = nullptr);
    ~PageHost() override = default;

    [[nodiscard]] QWidget* EnsurePage(const QString& page_key, const PageRegistry& registry);

    void SetMode(PageHostMode mode);
    void ActivateSingle(const QString& page_key, QWidget* page);
    QWidget* ActivateTab(const QString& page_key, QWidget* page, const TabOptions& options);
    void CloseTab(int index);
    [[nodiscard]] QByteArray SaveSession() const;
    [[nodiscard]] bool RestoreSession(const QByteArray& state, const PageRegistry& registry);

    [[nodiscard]] QWidget* PageForKey(const QString& page_key) const;
    [[nodiscard]] QWidget* PageForTab(int index) const;
    [[nodiscard]] QString KeyForTab(int index) const;
    [[nodiscard]] QString KeyForWidget(QWidget* page) const;
    [[nodiscard]] QString CurrentPageKey() const;
    [[nodiscard]] QWidget* CurrentPage() const;
    [[nodiscard]] PageHostMode CurrentMode() const;

   signals:
    void SigCurrentPageChanged(const QString& page_key);

   private:
    void SetCurrentPage(const QString& page_key, QWidget* page);
    void OnCurrentTabChanged(int index);
    [[nodiscard]] bool CanClosePage(const QString& page_key, QWidget* page) const;
    void ClearTrackedPages();
    void TrackPage(const QString& page_key, QWidget* page);
    void RemoveTrackedPage(const QString& page_key, QWidget* page);
    [[nodiscard]] int TabIndexOf(QWidget* page) const;
    [[nodiscard]] static QByteArray SavePageState(QWidget* page);
    static void RestorePageState(QWidget* page, const QByteArray& state);
    static void NotifyActivated(QWidget* page);
    static void NotifyDeactivated(QWidget* page);

    QStackedWidget* stacked_{nullptr};
    QTabWidget* tabs_{nullptr};
    PageHostMode mode_{PageHostMode::kSingle};
    QHash<QString, QPointer<QWidget>> pages_;
    QHash<QWidget*, QString> page_keys_;
    QHash<QWidget*, bool> tab_closable_;
    QString current_page_key_;
    QPointer<QWidget> current_page_;
};

}  // namespace app::ui::workspace
