# app/CMakeLists.txt
set(APP_TARGET "${APP_SLUG}")

add_executable(${APP_TARGET}
    app_bootstrap.h
    app_bootstrap.cpp
    app_context.h
    app_context.cpp
    app_info.h
    app_info.cpp
    app_services.h
    app_services.cpp
    app_translator.h
    app_translator.cpp
    document_session.h
    document_session.cpp
    commands/command.h
    commands/command.cpp
    commands/command_ids.h
    commands/command_registry.h
    commands/command_registry.cpp
    controllers/app_command_controller.h
    controllers/app_command_controller.cpp
    controllers/navigation_controller.h
    controllers/navigation_controller.cpp
    controllers/page_info_panel_controller.h
    controllers/page_info_panel_controller.cpp
    controllers/workspace_controller.h
    controllers/workspace_controller.cpp
    client_gateway/client_gateway.h
    client_gateway/client_gateway.cpp
    client_gateway/client_gateway_types.h
    client_gateway/drone_gateway_client.h
    client_gateway/mission_workspace_client.h
    client_gateway/swarm_runtime_client.h
    client_gateway/client_mission_workspace_gateway.h
    client_gateway/client_mission_workspace_gateway.cpp
    client_gateway/simulated_client_gateway.h
    client_gateway/simulated_client_gateway.cpp
    client_gateway/simulated_mission_workspace_client.h
    client_gateway/simulated_mission_workspace_client.cpp
    client_gateway/simulated_drone_gateway_client.h
    client_gateway/simulated_drone_gateway_client.cpp
    client_gateway/simulated_swarm_runtime_client.h
    client_gateway/simulated_swarm_runtime_client.cpp
    client_gateway/api_gateway_client.h
    client_gateway/api_gateway_client.cpp
    modules/app_module.h
    modules/app_module.cpp
    modules/mission_workspace_module.h
    modules/mission_workspace_module.cpp
    mission/mission_workspace.h
    mission/mission_workspace_gateway.h
    mission/in_memory_mission_workspace_gateway.h
    mission/in_memory_mission_workspace_gateway.cpp
    mission/mission_workspace_service.h
    mission/mission_workspace_service.cpp
    mission/ui/map_selection_page.h
    mission/ui/map_selection_page.cpp
    mission/ui/graph_editor_page.h
    mission/ui/graph_editor_page.cpp
    mission/ui/simulation_page.h
    mission/ui/simulation_page.cpp
    mission/ui/mission_live_page.h
    mission/ui/mission_live_page.cpp
    mission/ui/workspace_browser_dialog.h
    mission/ui/workspace_browser_dialog.cpp
    mission/ui/workspace_view_fit.h
    mission/ui/workspace_view_fit.cpp
    main.cpp
    main_window.h
    main_window.cpp
    main_window_layout_profile.h
    main_window_layout_profile.cpp
    shell_layout_manager.h
    shell_layout_manager.cpp
    ../resources/ui_theme.qrc
    ../packaging/windows/app.rc
)

set_target_properties(${APP_TARGET} PROPERTIES
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)

# Windows GUI app (no console)
if(WIN32)
    set_target_properties(${APP_TARGET} PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

# macOS .app bundle
if(APPLE)
    set_target_properties(${APP_TARGET} PROPERTIES
        MACOSX_BUNDLE TRUE
    )
endif()

# Linux installers bundle Qt shared libraries beside the app under ../lib.
# Keep the installed binary relocatable for /opt, $HOME, and custom IFW targets.
if(UNIX AND NOT APPLE)
    set_target_properties(${APP_TARGET} PROPERTIES
        INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}"
    )
endif()

target_include_directories(${APP_TARGET}
    PRIVATE
        "${CMAKE_SOURCE_DIR}"
)

target_link_libraries(${APP_TARGET}
    PRIVATE
        project_options
        project_warnings
        Qt6::Widgets
        Qt6::WebChannel
        Qt6::WebEngineWidgets
        app_ui
)

app_enable_sanitizers(${APP_TARGET})

qt_add_translations(${APP_TARGET}
    SOURCE_TARGETS
        ${APP_TARGET}
        app_ui
        app_core
    TS_FILES
        "${CMAKE_SOURCE_DIR}/resources/translations/${APP_SLUG}_ru.ts"
    RESOURCE_PREFIX "/translations"
    LUPDATE_TARGET update_translations
    LRELEASE_TARGET release_translations
)

# Single source of truth: app identity via compile definitions
target_compile_definitions(${APP_TARGET}
    PRIVATE
        APP_DISPLAY_NAME="${APP_DISPLAY_NAME}"
        APP_SLUG="${APP_SLUG}"
        APP_ID="${APP_ID}"
        APP_VENDOR="${APP_VENDOR}"
        APP_VERSION="${APP_VERSION}"
        APP_RELEASE_DATE="${APP_RELEASE_DATE}"
        SWARMOPS_CLIENT_GATEWAY_SIMULATOR=$<BOOL:${SWARMOPS_CLIENT_GATEWAY_SIMULATOR}>
)

set(APP_TARGET "${APP_TARGET}" PARENT_SCOPE)
