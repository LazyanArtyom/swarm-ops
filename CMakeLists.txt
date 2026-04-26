cmake_minimum_required(VERSION 3.28)
project(SwarmOps VERSION 1.0.0 LANGUAGES CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# =========================================================
# Build output layout (keep binaries at build root)
# =========================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")

# Multi-config generators (VS/Xcode) — keep outputs stable
foreach(cfg IN ITEMS Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${cfg}" CFG_UP)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${CFG_UP} "${CMAKE_BINARY_DIR}")
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${CFG_UP} "${CMAKE_BINARY_DIR}")
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${CFG_UP} "${CMAKE_BINARY_DIR}")
endforeach()

# =========================================================
# App identity
# =========================================================
set(APP_DISPLAY_NAME "SwarmOps" CACHE STRING "Human-readable application name")
set(APP_SLUG         "swarmops" CACHE STRING "Executable/filename slug (lowercase, no spaces)")
set(APP_ID           "com.swarmops.desktop" CACHE STRING "Reverse-DNS application id (also IFW package id)")
set(APP_VENDOR       "SwarmOps" CACHE STRING "Vendor/Company name")
set(APP_TARGET       "${APP_SLUG}")

# =========================================================
# Tool locations (provided via presets)
# =========================================================
set(QT_ROOT    "" CACHE PATH "Qt install prefix (Linux: .../gcc_64, Windows: .../mingw_64, macOS: .../macos)")
set(QTIFW_ROOT "" CACHE PATH "Qt Installer Framework root (contains bin/binarycreator(.exe))")
set(MINGW_ROOT "" CACHE PATH "MinGW root (Windows; contains bin/g++.exe etc.)")
set(APP_DIST_DIR "${CMAKE_SOURCE_DIR}/dist" CACHE PATH "Directory for final packaged artifacts")
option(SWARMOPS_CLIENT_GATEWAY_SIMULATOR
       "Use the local client API Gateway simulator instead of the real gRPC adapter placeholder"
       ON)

# Make Qt discoverable
if(QT_ROOT)
    list(PREPEND CMAKE_PREFIX_PATH "${QT_ROOT}")
endif()

# Helpful for tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# =========================================================
# Versioning artifacts
# =========================================================
set(APP_VERSION "${PROJECT_VERSION}")
string(TIMESTAMP APP_RELEASE_DATE "%Y-%m-%d")

# =========================================================
# Common build options (warnings, C++ standard, sanitizers)
# =========================================================
include(GNUInstallDirs)
include(ProjectOptions)
include(CTest)

# =========================================================
# Conan — auto-install dependencies on configure
# =========================================================
include(ConanSetup)
app_setup_conan()

# =========================================================
# Dependencies
# =========================================================
set(QT_MIN_VERSION 6.8)
find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Core Gui Widgets WebChannel WebEngineWidgets LinguistTools)
find_package(spdlog CONFIG REQUIRED)

# =========================================================
# Sanity checks
# =========================================================
if(NOT QT_ROOT OR QT_ROOT STREQUAL "")
    message(WARNING "QT_ROOT is empty. Qt will be found via default CMAKE_PREFIX_PATH/system. "
                    "Presets should set QT_ROOT for reproducibility.")
endif()

if(WIN32)
    if(QT_ROOT AND NOT EXISTS "${QT_ROOT}/bin/windeployqt.exe")
        message(FATAL_ERROR "Invalid QT_ROOT (windeployqt.exe not found): ${QT_ROOT}")
    endif()
    if(QTIFW_ROOT AND NOT QTIFW_ROOT STREQUAL "" AND NOT EXISTS "${QTIFW_ROOT}/bin/binarycreator.exe")
        message(FATAL_ERROR "Invalid QTIFW_ROOT (binarycreator.exe not found): ${QTIFW_ROOT}")
    endif()
    if(NOT MINGW_ROOT OR MINGW_ROOT STREQUAL "")
        message(WARNING "MINGW_ROOT is empty. Windows installer may miss runtime DLLs.")
    endif()
endif()

if(UNIX AND QTIFW_ROOT AND NOT QTIFW_ROOT STREQUAL "")
    if(NOT EXISTS "${QTIFW_ROOT}/bin/binarycreator")
        message(WARNING "Invalid QTIFW_ROOT (binarycreator not found): ${QTIFW_ROOT}. "
                        "Linux installer packaging will also try PATH and "
                        "$HOME/Qt/Tools/QtInstallerFramework/*/bin/binarycreator.")
    endif()
endif()

# =========================================================
# Packaging artifact generation
# =========================================================
include(Packaging)
app_packaging_generate()

# =========================================================
# Subdirectories
# =========================================================
add_subdirectory(core)
add_subdirectory(ui)
add_subdirectory(app)
if(BUILD_TESTING)
    add_subdirectory(tests)
endif()

# =========================================================
# Install layout (cross-platform)
# =========================================================
install(TARGETS ${APP_TARGET}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}   # Linux/Windows
    BUNDLE  DESTINATION .                          # macOS .app
)

install(DIRECTORY resources/
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${APP_SLUG}/resources
    FILES_MATCHING
        PATTERN "*.qss"
        PATTERN "*.svg"
        PATTERN "*.png"
        PATTERN "*.ttf"
)

# Linux desktop integration
if(UNIX AND NOT APPLE)
    install(FILES
        "${CMAKE_CURRENT_BINARY_DIR}/packaging/linux/${APP_SLUG}.desktop"
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/applications
        RENAME "${APP_SLUG}.desktop"
    )
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icons/app_icon_linux.png")
        install(FILES
            "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icons/app_icon_linux.png"
            DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/512x512/apps
            RENAME "${APP_SLUG}.png"
        )
    endif()

    qt_generate_deploy_app_script(
        TARGET ${APP_TARGET}
        OUTPUT_SCRIPT app_linux_deploy_script
        NO_TRANSLATIONS
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT "${app_linux_deploy_script}")
endif()

# Windows icon install
if(WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icons/app_icon_win.ico")
    install(FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/packaging/icons/app_icon_win.ico"
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/icons
        RENAME "${APP_SLUG}.ico"
    )
endif()
