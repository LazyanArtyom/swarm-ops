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
    controllers/workspace_controller.h
    controllers/workspace_controller.cpp
    modules/app_module.h
    modules/app_module.cpp
    modules/home_module.h
    modules/home_module.cpp
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
)

set(APP_TARGET "${APP_TARGET}" PARENT_SCOPE)
