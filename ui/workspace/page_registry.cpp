#include "workspace/page_registry.h"

#include <QWidget>
#include <utility>

namespace app::ui::workspace {

void PageRegistry::RegisterFactory(const QString& page_key, PageFactory factory) {
    RegisterPage({
        .page_key = page_key,
        .title = page_key,
        .factory = std::move(factory),
        .navigation_visible = false,
    });
}

void PageRegistry::RegisterPage(PageDescriptor descriptor) {
    if (descriptor.page_key.isEmpty() || !descriptor.factory) {
        return;
    }

    if (!pages_.contains(descriptor.page_key)) {
        order_.push_back(descriptor.page_key);
    }
    if (descriptor.title.isEmpty()) {
        descriptor.title = descriptor.page_key;
    }
    pages_[descriptor.page_key] = std::move(descriptor);
}

void PageRegistry::UnregisterFactory(const QString& page_key) {
    pages_.remove(page_key);
    order_.removeAll(page_key);
}

bool PageRegistry::HasFactory(const QString& page_key) const {
    return pages_.contains(page_key);
}

QWidget* PageRegistry::CreatePage(const QString& page_key, QWidget* parent) const {
    const auto page_it = pages_.find(page_key);
    if (page_it == pages_.end() || !page_it->factory) {
        return nullptr;
    }

    return page_it->factory(parent);
}

PageDescriptor PageRegistry::Descriptor(const QString& page_key) const {
    return pages_.value(page_key);
}

QList<PageDescriptor> PageRegistry::Pages() const {
    QList<PageDescriptor> descriptors;
    descriptors.reserve(order_.size());

    for (const QString& page_key : order_) {
        const auto page_it = pages_.find(page_key);
        if (page_it != pages_.end()) {
            descriptors.push_back(page_it.value());
        }
    }

    return descriptors;
}

QList<PageDescriptor> PageRegistry::NavigationPages() const {
    QList<PageDescriptor> descriptors;
    for (const PageDescriptor& descriptor : Pages()) {
        if (descriptor.navigation_visible) {
            descriptors.push_back(descriptor);
        }
    }
    return descriptors;
}

QString PageRegistry::DefaultPageKey() const {
    const QList<PageDescriptor> navigation_pages = NavigationPages();
    if (!navigation_pages.isEmpty()) {
        return navigation_pages.front().page_key;
    }

    const QList<PageDescriptor> pages = Pages();
    return pages.isEmpty() ? QString() : pages.front().page_key;
}

}  // namespace app::ui::workspace
