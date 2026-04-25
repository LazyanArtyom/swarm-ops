#pragma once

#include <QObject>

namespace app::ui::theme {

struct ThemeMetricsData final {
    int font_base_pt = 0;
    int font_mono_pt = 0;
    int font_tooltip_pt = 0;
    int radius_sm_px = 0;
    int radius_md_px = 0;
    int spacing_xs_px = 0;
    int spacing_sm_px = 0;
    int spacing_md_px = 0;
    int spacing_lg_px = 0;
    int icon_sm_px = 0;
    int icon_md_px = 0;
    int control_height_px = 0;
    int navigation_width_px = 0;
    int navigation_min_width_px = 0;
    int central_width_px = 0;
    int central_min_width_px = 0;
    int info_width_px = 0;
    int info_min_width_px = 0;
    int workspace_height_px = 0;
    int console_height_px = 0;
    int console_min_height_px = 0;
    int main_window_default_width_px = 0;
    int main_window_default_height_px = 0;
    int main_window_min_width_px = 0;
    int main_window_min_height_px = 0;
    double main_window_max_screen_width_ratio = 0.0;
    double main_window_max_screen_height_ratio = 0.0;
    int dialog_icon_px = 0;
    int dialog_message_min_width_px = 0;
    int dialog_details_min_height_px = 0;
    int dialog_button_min_height_px = 0;
    int dialog_button_min_width_px = 0;
    int dialog_button_padding_x_px = 0;
    int dialog_button_padding_y_px = 0;
    int settings_dialog_min_width_px = 0;
    int settings_dialog_min_height_px = 0;
    int settings_navigation_width_px = 0;
    int settings_label_width_px = 0;
    int settings_field_min_width_px = 0;
    int settings_field_height_px = 0;
    int settings_popup_min_height_px = 0;
    int console_header_control_height_px = 0;
    int console_level_filter_min_width_px = 0;
    int console_search_min_width_px = 0;
    int console_search_max_width_px = 0;
    int panel_close_button_extent_px = 0;
    int scrollbar_extent_px = 0;
    int scrollbar_min_slider_px = 0;
    int tooltip_padding_x_px = 0;
    int tooltip_padding_y_px = 0;
    int toolbar_button_padding_x_px = 0;
    int toolbar_button_padding_y_px = 0;

    friend bool operator==(const ThemeMetricsData&, const ThemeMetricsData&) = default;
};

class ThemeMetrics final : public QObject {
    Q_OBJECT

   public:
    static ThemeMetrics& Instance();
    [[nodiscard]] ThemeMetricsData Current() const;
    void Refresh();

   signals:
    void SigMetricsChanged();

   private:
    ThemeMetrics() = default;
    ThemeMetricsData metrics_;
};

}  // namespace app::ui::theme
