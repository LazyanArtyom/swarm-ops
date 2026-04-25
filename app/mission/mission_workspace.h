#pragma once

#include <QImage>
#include <QList>
#include <QPointF>
#include <QString>

namespace app::mission {

enum class GraphNodeType {
    kBorder,
    kCorner,
};

enum class GraphNodeCategory {
    kGeneric,
    kDrone,
    kAttacker,
    kTarget,
};

struct MapBounds final {
    double north{0.0};
    double west{0.0};
    double south{0.0};
    double east{0.0};
};

struct WorkspaceBackground final {
    QImage image;
    MapBounds bounds;

    [[nodiscard]] bool IsValid() const {
        return !image.isNull();
    }
};

struct GraphNode final {
    QString id;
    QString label;
    QPointF position;
    GraphNodeType type{GraphNodeType::kBorder};
    GraphNodeCategory category{GraphNodeCategory::kGeneric};
};

struct GraphEdge final {
    QString id;
    QString from_node_id;
    QString to_node_id;
};

struct MissionWorkspace final {
    QString id;
    QString name;
    WorkspaceBackground background;
    QList<GraphNode> nodes;
    QList<GraphEdge> edges;
};

}  // namespace app::mission
