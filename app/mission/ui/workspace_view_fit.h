#pragma once

#include <QImage>
#include <QSizeF>

class QGraphicsPixmapItem;
class QGraphicsScene;
class QGraphicsView;

namespace app::mission::view_fit {

[[nodiscard]] QSizeF WorkspaceImageSize(const QImage& image);
[[nodiscard]] QGraphicsPixmapItem* AddWorkspaceBackground(QGraphicsScene& scene,
                                                          const QImage& image);
[[nodiscard]] bool ApplyExactSceneFit(QGraphicsView& view, const QGraphicsScene& scene);

}  // namespace app::mission::view_fit
