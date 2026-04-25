# ui/CMakeLists.txt
add_library(app_ui STATIC)

target_sources(app_ui PRIVATE
    # top
    ids.h
    object_names.h

    # actions
    actions/app_actions.h
    actions/app_actions.cpp

    # chrome
    chrome/main_menu_bar.h
    chrome/main_menu_bar.cpp
    chrome/main_tool_bar.h
    chrome/main_tool_bar.cpp

    # logging
    logging/console_log_bridge.h
    logging/console_log_bridge.cpp

    # workspace
    workspace/page_registry.h
    workspace/page_registry.cpp
    workspace/page_lifecycle.h
    workspace/page_lifecycle.cpp
    workspace/page_host.h
    workspace/page_host.cpp

    # theme
    theme/theme_colors.h
    theme/theme_colors.cpp
    theme/theme_loader.h
    theme/theme_loader.cpp
    theme/theme_manager.h
    theme/theme_manager.cpp
    theme/fonts_config.h
    theme/fonts_config.cpp
    theme/theme_metrics.h
    theme/theme_metrics.cpp
    theme/theme_icons.h
    theme/theme_icons.cpp

    # panels
    panels/panel_widget.h
    panels/panel_widget.cpp
    panels/console_panel.h
    panels/console_panel.cpp
    panels/central_panel.h
    panels/central_panel.cpp
    panels/info_panel.h
    panels/info_panel.cpp
    panels/navigation_panel.h
    panels/navigation_panel.cpp

    # dialogs
    dialogs/about_dialog.h
    dialogs/about_dialog.cpp
    dialogs/settings_dialog.h
    dialogs/settings_dialog.cpp
    dialogs/message_dialog.h
    dialogs/message_dialog.cpp

    # widgets
    widgets/settings_form_row.h
    widgets/settings_form_row.cpp
)

target_include_directories(app_ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(app_ui
    PUBLIC
        project_options
        app_core
        Qt6::Widgets
    PRIVATE
        project_warnings
)

app_enable_sanitizers(app_ui)

set_target_properties(app_ui PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)
