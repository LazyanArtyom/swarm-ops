#include "app/mission/ui/map_selection_page.h"

#include <QTimer>
#include <QVBoxLayout>
#include <QWebChannel>
#include <QWebEngineSettings>
#include <QWebEngineView>
#include <QtMath>
#include <algorithm>

#include "app/mission/mission_workspace_service.h"
#include "ui/theme/theme_metrics.h"

namespace app::mission {

namespace {

constexpr int kMinimumCaptureWidthPx = 1600;
constexpr int kMinimumCaptureHeightPx = 900;
constexpr int kMaximumCaptureLongSidePx = 2600;

QImage NormalizeEditorCapture(const QPixmap& screenshot) {
    QImage image = screenshot.toImage();
    if (image.isNull()) {
        return {};
    }

    const QSize original_size = image.size();
    const double minimum_scale =
        std::max(static_cast<double>(kMinimumCaptureWidthPx) / original_size.width(),
                 static_cast<double>(kMinimumCaptureHeightPx) / original_size.height());
    const double maximum_scale =
        static_cast<double>(kMaximumCaptureLongSidePx) /
        static_cast<double>(std::max(original_size.width(), original_size.height()));
    const double scale = std::max(1.0, std::min(minimum_scale, maximum_scale));
    if (scale <= 1.0) {
        return image;
    }

    return image.scaled(original_size * scale, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

}  // namespace

MapSelectionBridge::MapSelectionBridge(QObject* parent) : QObject(parent) {}

void MapSelectionBridge::areaSelected(double north, double west, double south, double east,
                                      double x, double y, double width, double height) {
    QRect capture_rect(qRound(x), qRound(y), qRound(width), qRound(height));
    capture_rect = capture_rect.normalized();
    emit SigAreaSelected({.north = north, .west = west, .south = south, .east = east},
                         capture_rect);
}

MapSelectionPage::MapSelectionPage(QWidget* parent) : QWidget(parent) {
    const auto metrics = ui::theme::ThemeMetrics::Instance().Current();

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(metrics.spacing_md_px, metrics.spacing_md_px, metrics.spacing_md_px,
                               metrics.spacing_md_px);
    layout->setSpacing(metrics.spacing_sm_px);

    web_view_ = new QWebEngineView(this);
    web_view_->setContextMenuPolicy(Qt::NoContextMenu);
    web_view_->settings()->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    web_view_->settings()->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);

    bridge_ = new MapSelectionBridge(this);
    web_channel_ = new QWebChannel(this);
    web_channel_->registerObject(QStringLiteral("mapBridge"), bridge_);
    web_view_->page()->setWebChannel(web_channel_);

    layout->addWidget(web_view_, 1);

    connect(bridge_, &MapSelectionBridge::SigAreaSelected, this, &MapSelectionPage::OnAreaSelected);

    web_view_->setHtml(MapHtml(), QUrl(QStringLiteral("https://swarmops.local/")));
}

void MapSelectionPage::OnAreaSelected(const MapBounds& bounds, const QRect& capture_rect) {
    QTimer::singleShot(180, this, [this, bounds, capture_rect] {
        CaptureWorkspace(bounds, capture_rect);
    });
}

void MapSelectionPage::CaptureWorkspace(const MapBounds& bounds, const QRect& capture_rect) {
    if (web_view_ == nullptr) {
        return;
    }

    const QRect safe_rect = capture_rect.intersected(web_view_->rect());
    if (!safe_rect.isValid() || safe_rect.width() < 8 || safe_rect.height() < 8) {
        return;
    }

    const QPixmap screenshot = web_view_->grab(safe_rect);
    if (screenshot.isNull()) {
        return;
    }

    WorkspaceBackground background;
    background.image = NormalizeEditorCapture(screenshot);
    background.bounds = bounds;
    MissionWorkspaceRuntime().CreateWorkspaceFromMapCapture(background);
}

QString MapSelectionPage::MapHtml() {
    return QStringLiteral(R"HTML(
<!doctype html>
<html>
<head>
  <meta charset="utf-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>SwarmOps Map Selection</title>
  <link rel="stylesheet" href="https://unpkg.com/leaflet@1.9.4/dist/leaflet.css">
  <script src="qrc:///qtwebchannel/qwebchannel.js"></script>
  <script src="https://unpkg.com/leaflet@1.9.4/dist/leaflet.js"></script>
  <style>
    html, body, #map { width: 100%; height: 100%; margin: 0; overflow: hidden; }
    body { background: #1f2125; }
    #map { cursor: grab; user-select: none; }
    #map.selecting { cursor: crosshair; }
    #selectionLayer {
      position: absolute;
      inset: 0;
      z-index: 1000;
      pointer-events: none;
    }
    #selectionLayer.active {
      pointer-events: auto;
      cursor: crosshair;
    }
    #selectionBox {
      position: absolute;
      display: none;
      border: 2px solid #7cc7ff;
      background: rgba(66, 133, 244, 0.20);
      box-sizing: border-box;
      pointer-events: none;
    }
    #searchPanel {
      position: absolute;
      top: 12px;
      left: 54px;
      z-index: 900;
      display: flex;
      align-items: center;
      gap: 6px;
      min-width: 300px;
      max-width: min(460px, calc(100vw - 92px));
      padding: 6px;
      border: 1px solid rgba(255, 255, 255, 0.14);
      border-radius: 6px;
      background: rgba(24, 28, 32, 0.92);
      box-shadow: 0 10px 28px rgba(0, 0, 0, 0.28);
    }
    #searchInput {
      flex: 1;
      min-width: 0;
      height: 28px;
      padding: 0 10px;
      border: 1px solid rgba(255, 255, 255, 0.14);
      border-radius: 4px;
      outline: none;
      color: #e8eaed;
      background: #20262d;
      font: 13px system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    }
    #searchInput:focus {
      border-color: #7cc7ff;
    }
    #searchButton {
      height: 28px;
      padding: 0 11px;
      border: 0;
      border-radius: 4px;
      color: #101418;
      background: #7cc7ff;
      font: 600 13px system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
      cursor: pointer;
    }
    #searchButton:disabled {
      opacity: 0.55;
      cursor: default;
    }
    #searchStatus {
      position: absolute;
      top: 56px;
      left: 54px;
      z-index: 900;
      display: none;
      max-width: min(460px, calc(100vw - 92px));
      padding: 6px 9px;
      border-radius: 4px;
      color: #e8eaed;
      background: rgba(24, 28, 32, 0.90);
      font: 12px system-ui, -apple-system, BlinkMacSystemFont, "Segoe UI", sans-serif;
    }
    body.capture-mode #selectionLayer,
    body.capture-mode #selectionBox,
    body.capture-mode #searchPanel,
    body.capture-mode #searchStatus {
      display: none !important;
    }
    .leaflet-control-attribution { display: none; }
  </style>
</head>
<body>
  <div id="map"></div>
  <div id="searchPanel">
    <input id="searchInput" type="text" placeholder="Search location" autocomplete="off">
    <button id="searchButton" type="button">Search</button>
  </div>
  <div id="searchStatus"></div>
  <div id="selectionLayer"><div id="selectionBox"></div></div>
  <script>
    let bridge = null;
    new QWebChannel(qt.webChannelTransport, function(channel) {
      bridge = channel.objects.mapBridge;
    });

    const map = L.map('map', {
      zoomControl: true,
      attributionControl: false,
      center: [40.1792, 44.4991],
      zoom: 13
    });

    L.tileLayer('https://server.arcgisonline.com/ArcGIS/rest/services/World_Imagery/MapServer/tile/{z}/{y}/{x}', {
      maxZoom: 19,
      attribution: 'Tiles © Esri',
      crossOrigin: true
    }).addTo(map);

    L.tileLayer('https://services.arcgisonline.com/ArcGIS/rest/services/Reference/World_Boundaries_and_Places/MapServer/tile/{z}/{y}/{x}', {
      maxZoom: 19,
      opacity: 0.85,
      attribution: 'Labels © Esri',
      crossOrigin: true
    }).addTo(map);

    const mapElement = document.getElementById('map');
    const searchPanel = document.getElementById('searchPanel');
    const searchInput = document.getElementById('searchInput');
    const searchButton = document.getElementById('searchButton');
    const searchStatus = document.getElementById('searchStatus');
    const layer = document.getElementById('selectionLayer');
    const box = document.getElementById('selectionBox');

    let drawing = false;
    let startPoint = null;
    let lastPoint = null;
    let searchMarker = null;

    function setSearchStatus(message) {
      if (!message) {
        searchStatus.style.display = 'none';
        searchStatus.textContent = '';
        return;
      }
      searchStatus.textContent = message;
      searchStatus.style.display = 'block';
    }

    async function searchLocation() {
      const query = searchInput.value.trim();
      if (!query) {
        searchInput.focus();
        return;
      }

      searchButton.disabled = true;
      setSearchStatus('Searching...');

      try {
        const response = await fetch(
          'https://nominatim.openstreetmap.org/search?format=json&limit=1&q=' +
          encodeURIComponent(query),
          { headers: { 'Accept': 'application/json' } }
        );

        if (!response.ok) {
          throw new Error('Search request failed');
        }

        const results = await response.json();
        if (!Array.isArray(results) || results.length === 0) {
          setSearchStatus('No location found');
          return;
        }

        const result = results[0];
        const lat = Number(result.lat);
        const lon = Number(result.lon);
        if (!Number.isFinite(lat) || !Number.isFinite(lon)) {
          setSearchStatus('Invalid location result');
          return;
        }

        map.setView([lat, lon], Math.max(map.getZoom(), 15));
        if (searchMarker) {
          searchMarker.setLatLng([lat, lon]);
        } else {
          searchMarker = L.marker([lat, lon]).addTo(map);
        }
        setSearchStatus(result.display_name || 'Location found');
        window.setTimeout(function() { setSearchStatus(''); }, 3200);
      } catch (error) {
        setSearchStatus('Search failed');
      } finally {
        searchButton.disabled = false;
      }
    }

    searchButton.addEventListener('click', searchLocation);
    searchInput.addEventListener('keydown', function(event) {
      if (event.key === 'Enter') {
        event.preventDefault();
        searchLocation();
      }
      event.stopPropagation();
    });

    searchPanel.addEventListener('mousedown', function(event) {
      event.stopPropagation();
    });

    function enableSelection() {
      map.dragging.disable();
      mapElement.classList.add('selecting');
      layer.classList.add('active');
    }

    function disableSelection() {
      if (drawing) return;
      map.dragging.enable();
      mapElement.classList.remove('selecting');
      layer.classList.remove('active');
    }

    function normalize(a, b) {
      const left = Math.min(a.x, b.x);
      const top = Math.min(a.y, b.y);
      const right = Math.max(a.x, b.x);
      const bottom = Math.max(a.y, b.y);
      return { x: left, y: top, width: right - left, height: bottom - top };
    }

    function drawBox() {
      const rect = normalize(startPoint, lastPoint);
      box.style.display = 'block';
      box.style.left = rect.x + 'px';
      box.style.top = rect.y + 'px';
      box.style.width = rect.width + 'px';
      box.style.height = rect.height + 'px';
    }

    function eventPoint(event) {
      const rect = mapElement.getBoundingClientRect();
      return L.point(event.clientX - rect.left, event.clientY - rect.top);
    }

    document.addEventListener('keydown', function(event) {
      if (event.key === 'Control') enableSelection();
    });

    document.addEventListener('keyup', function(event) {
      if (event.key === 'Control') disableSelection();
    });

    window.addEventListener('blur', disableSelection);
    document.addEventListener('contextmenu', function(event) {
      event.preventDefault();
      event.stopPropagation();
    }, true);

    layer.addEventListener('mousedown', function(event) {
      if (!event.ctrlKey) return;
      event.preventDefault();
      event.stopPropagation();
      drawing = true;
      startPoint = eventPoint(event);
      lastPoint = startPoint;
      enableSelection();
      drawBox();
    });

    layer.addEventListener('mousemove', function(event) {
      if (!drawing) return;
      event.preventDefault();
      event.stopPropagation();
      lastPoint = eventPoint(event);
      drawBox();
    });

    document.addEventListener('mouseup', function(event) {
      if (!drawing) return;
      event.preventDefault();
      event.stopPropagation();
      drawing = false;
      lastPoint = eventPoint(event);
      const rect = normalize(startPoint, lastPoint);

      box.style.display = 'none';
      document.body.classList.add('capture-mode');
      disableSelection();

      if (rect.width < 8 || rect.height < 8 || !bridge) {
        document.body.classList.remove('capture-mode');
        return;
      }

      const nw = map.containerPointToLatLng(L.point(rect.x, rect.y));
      const se = map.containerPointToLatLng(L.point(rect.x + rect.width, rect.y + rect.height));
      bridge.areaSelected(nw.lat, nw.lng, se.lat, se.lng, rect.x, rect.y, rect.width, rect.height);
      window.setTimeout(function() {
        document.body.classList.remove('capture-mode');
      }, 500);
    });
  </script>
</body>
</html>
)HTML");
}

}  // namespace app::mission
