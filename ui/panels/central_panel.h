#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QWidget>

#include "workspace/page_registry.h"

class QStackedWidget;
class QTabWidget;

namespace app::ui {

namespace workspace {
class PageHost;
}  // namespace workspace

class CentralPanel final : public QWidget {
    Q_OBJECT

   public:
    enum class Mode : std::uint8_t {
        kSingle,
        kTabs,
    };

    struct OpenOptions final {
        workspace::PageOpenMode open_mode{workspace::PageOpenMode::kSingle};
        bool single_instance{true};
        QString title;
    };

    using PageFactory = workspace::PageFactory;

    explicit CentralPanel(QWidget* parent = nullptr);
    CentralPanel(const CentralPanel&) = delete;
    CentralPanel& operator=(const CentralPanel&) = delete;
    ~CentralPanel() override = default;

    void SetMode(Mode mode);
    [[nodiscard]] Mode CurrentMode() const;

    void RegisterPageFactory(const QString& page_key, PageFactory factory);
    void UnregisterPageFactory(const QString& page_key);
    void SetPageRegistry(workspace::PageRegistry* page_registry);
    [[nodiscard]] QByteArray SaveSession() const;
    [[nodiscard]] bool RestoreSession(const QByteArray& state);

    QWidget* ShowPage(const QString& page_key);  // kSingle: show, kTabs: activate tab if exists.
    QWidget* OpenPage(const QString& page_key, const OpenOptions& options);

    QWidget* PushPage(const QString& page_key);
    void PopPage();

    [[nodiscard]] bool CanGoBack() const;
    [[nodiscard]] QString CurrentPageKey() const;
    [[nodiscard]] QWidget* CurrentPage() const;

   signals:
    void SigCurrentPageChanged(const QString& page_key);
    void SigBackAvailabilityChanged(bool can_go_back);

   private:
    void ActivateSingle(const QString& page_key, QWidget* page);
    QWidget* ActivateTab(const QString& page_key, QWidget* page, const OpenOptions& options);
    void CloseTab(int index);

    void RebuildForMode(Mode mode);
    void UpdateBackSignal();
    [[nodiscard]] OpenOptions DefaultOpenOptions(const QString& page_key) const;

    Mode mode_{Mode::kSingle};

    QStackedWidget* stacked_{nullptr};
    QTabWidget* tabs_{nullptr};

    workspace::PageRegistry owned_page_registry_;
    workspace::PageRegistry* page_registry_{&owned_page_registry_};
    workspace::PageHost* page_host_{nullptr};

    QStringList nav_stack_;
};

}  // namespace app::ui
