# core/CMakeLists.txt
add_library(app_core STATIC
    configs/app_configs.h
    configs/app_configs.cpp
    logging/log_store.h
    logging/log_store.cpp
    logging/logger.h
    logging/logger.cpp
    logging/qt_message_handler.h
    logging/qt_message_handler.cpp
    utils/result.h
)

target_include_directories(app_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(app_core
    PUBLIC
        project_options
        Qt6::Core
        spdlog::spdlog
    PRIVATE
        project_warnings
)

app_enable_sanitizers(app_core)

set_target_properties(app_core PROPERTIES
    AUTOMOC ON
)
