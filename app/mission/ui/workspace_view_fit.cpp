#include "app/mission/ui/workspace_view_fit.h"

#include <QGraphicsPixmapItem>
#include <QGraphicsScene>
#include <QGraphicsView>
#include <QPixmap>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QTransform>

namespace app::mission::view_fit {

QSizeF WorkspaceImageSize(const QImage& image) {
    return image.isNull() ? QSizeF{} : QSizeF(image.width(), image.height());
}

QGraphicsPixmapItem* AddWorkspaceBackground(QGraphicsScene& scene, const QImage& image) {
    if (image.isNull()) {
        return nullptr;
    }

    QImage normalized_image = image;
    normalized_image.setDevicePixelRatio(1.0);

    QPixmap pixmap = QPixmap::fromImage(normalized_image);
    pixmap.setDevicePixelRatio(1.0);

    auto* item = scene.addPixmap(pixmap);
    item->setTransformationMode(Qt::SmoothTransformation);
    item->setZValue(0.0);

    const QSizeF target_size = WorkspaceImageSize(normalized_image);
    const QRectF item_rect = item->boundingRect();
    if (!target_size.isEmpty() && !item_rect.isEmpty() && item_rect.size() != target_size) {
        item->setTransform(QTransform::fromScale(target_size.width() / item_rect.width(),
                                                 target_size.height() / item_rect.height()));
    }

    scene.setSceneRect(QRectF(QPointF(0.0, 0.0), target_size));
    return item;
}

bool ApplyExactSceneFit(QGraphicsView& view, const QGraphicsScene& scene) {
    const QRectF scene_rect = scene.sceneRect();
    const QSize viewport_size = view.viewport()->size();
    if (scene_rect.isEmpty() || scene_rect.width() <= 0.0 || scene_rect.height() <= 0.0 ||
        viewport_size.width() <= 1 || viewport_size.height() <= 1) {
        return false;
    }

    const QSignalBlocker horizontal_blocker(view.horizontalScrollBar());
    const QSignalBlocker vertical_blocker(view.verticalScrollBar());
    view.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    view.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    const qreal x_scale = static_cast<qreal>(viewport_size.width()) / scene_rect.width();
    const qreal y_scale = static_cast<qreal>(viewport_size.height()) / scene_rect.height();
    view.setTransform(QTransform::fromScale(x_scale, y_scale));
    view.centerOn(scene_rect.center());
    return true;
}

}  // namespace app::mission::view_fit
