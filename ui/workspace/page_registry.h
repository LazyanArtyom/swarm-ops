#pragma once

#include <QHash>
#include <QIcon>
#include <QKeySequence>
#include <QList>
#include <QString>
#include <cstdint>
#include <functional>

class QWidget;

namespace app::ui::workspace {

using PageFactory = std::function<QWidget*(QWidget* parent)>;

enum class PageOpenMode : std::uint8_t {
    kSingle,
    kTab,
};

struct PageDescriptor final {
    QString page_key{};
    QString title{};
    QIcon icon{};
    QString category{};
    QKeySequence shortcut{};
    PageFactory factory;
    PageOpenMode default_open_mode{PageOpenMode::kSingle};
    bool closable{false};
    bool navigation_visible{true};
};

class PageRegistry final {
   public:
    void RegisterPage(PageDescriptor descriptor);
    void RegisterFactory(const QString& page_key, PageFactory factory);
    void UnregisterFactory(const QString& page_key);

    [[nodiscard]] bool HasFactory(const QString& page_key) const;
    [[nodiscard]] QWidget* CreatePage(const QString& page_key, QWidget* parent) const;
    [[nodiscard]] PageDescriptor Descriptor(const QString& page_key) const;
    [[nodiscard]] QList<PageDescriptor> Pages() const;
    [[nodiscard]] QList<PageDescriptor> NavigationPages() const;
    [[nodiscard]] QString DefaultPageKey() const;

   private:
    QHash<QString, PageDescriptor> pages_;
    QList<QString> order_;
};

}  // namespace app::ui::workspace
