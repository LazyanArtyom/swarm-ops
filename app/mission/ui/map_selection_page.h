#pragma once

#include <QObject>
#include <QRect>
#include <QWidget>

#include "app/mission/mission_workspace.h"

class QWebChannel;
class QWebEngineView;

namespace app::mission {

class MapSelectionBridge final : public QObject {
    Q_OBJECT

   public:
    explicit MapSelectionBridge(QObject* parent = nullptr);

   public slots:
    void areaSelected(double north, double west, double south, double east, double x, double y,
                      double width, double height);

   signals:
    void SigAreaSelected(app::mission::MapBounds bounds, QRect capture_rect);
};

class MapSelectionPage final : public QWidget {
    Q_OBJECT

   public:
    explicit MapSelectionPage(QWidget* parent = nullptr);

   private:
    void OnAreaSelected(const MapBounds& bounds, const QRect& capture_rect);
    void CaptureWorkspace(const MapBounds& bounds, const QRect& capture_rect);
    [[nodiscard]] static QString MapHtml();

    QWebEngineView* web_view_{nullptr};
    QWebChannel* web_channel_{nullptr};
    MapSelectionBridge* bridge_{nullptr};
};

}  // namespace app::mission
