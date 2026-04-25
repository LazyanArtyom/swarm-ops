find_package(Qt6 REQUIRED COMPONENTS Core Test Widgets)

add_executable(app_core_tests
    core/configs/app_configs_test.cpp
)

target_link_libraries(app_core_tests
    PRIVATE
        app_core
        project_options
        project_warnings
        Qt6::Core
        Qt6::Test
)

app_enable_sanitizers(app_core_tests)

set_target_properties(app_core_tests PROPERTIES
    AUTOMOC ON
)

add_test(NAME app_core_tests COMMAND app_core_tests)

add_executable(app_logger_tests
    core/logging/logger_test.cpp
)

target_link_libraries(app_logger_tests
    PRIVATE
        app_core
        project_options
        project_warnings
        Qt6::Core
        Qt6::Test
)

app_enable_sanitizers(app_logger_tests)

set_target_properties(app_logger_tests PROPERTIES
    AUTOMOC ON
)

add_test(NAME app_logger_tests COMMAND app_logger_tests)

add_executable(app_ui_tests
    ui/panels/central_panel_test.cpp
)

target_link_libraries(app_ui_tests
    PRIVATE
        app_ui
        project_options
        project_warnings
        Qt6::Core
        Qt6::Test
        Qt6::Widgets
)

app_enable_sanitizers(app_ui_tests)

set_target_properties(app_ui_tests PROPERTIES
    AUTOMOC ON
)

add_test(NAME app_ui_tests COMMAND app_ui_tests)

add_executable(app_theme_tests
    ui/theme/theme_system_test.cpp
)

target_link_libraries(app_theme_tests
    PRIVATE
        app_ui
        project_options
        project_warnings
        Qt6::Core
        Qt6::Test
        Qt6::Widgets
)

app_enable_sanitizers(app_theme_tests)

set_target_properties(app_theme_tests PROPERTIES
    AUTOMOC ON
)

add_test(NAME app_theme_tests COMMAND app_theme_tests)

if(WIN32)
    set(_test_runtime_path_mods)
    if(QT_ROOT)
        list(APPEND _test_runtime_path_mods "PATH=path_list_prepend:${QT_ROOT}/bin")
    endif()
    if(MINGW_ROOT)
        list(APPEND _test_runtime_path_mods "PATH=path_list_prepend:${MINGW_ROOT}/bin")
    endif()

    if(_test_runtime_path_mods)
        set_tests_properties(app_core_tests app_logger_tests app_ui_tests app_theme_tests PROPERTIES
            ENVIRONMENT_MODIFICATION "${_test_runtime_path_mods}"
        )
    endif()
endif()

if(UNIX AND NOT APPLE)
    set_tests_properties(app_ui_tests app_theme_tests PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
endif()
