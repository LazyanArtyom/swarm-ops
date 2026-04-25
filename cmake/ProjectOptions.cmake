# cmake/ProjectOptions.cmake
# Project-wide build options, compiler settings, and warning flags.

include_guard(GLOBAL)

# ---- Options ----
set(APP_CXX_STANDARD "23" CACHE STRING "C++ language standard for project targets (for example: 20, 23, 26)")
option(APP_WARNINGS_AS_ERRORS "Treat compiler warnings as errors" OFF)
option(APP_ENABLE_IPO       "Enable interprocedural optimization for release builds" OFF)
option(APP_ENABLE_ASAN       "Enable AddressSanitizer"           OFF)
option(APP_ENABLE_TSAN       "Enable ThreadSanitizer"            OFF)
option(APP_ENABLE_UBSAN      "Enable UndefinedBehaviorSanitizer" OFF)
option(APP_ENABLE_CLANG_TIDY "Run clang-tidy during build"       OFF)
option(APP_ENABLE_CCACHE     "Use ccache as compiler launcher"   OFF)
option(APP_ENABLE_SCCACHE    "Use sccache as compiler launcher"  OFF)

set_property(CACHE APP_CXX_STANDARD PROPERTY STRINGS 20 23 26)

# ---- Mutual exclusion guard ----
if(APP_ENABLE_ASAN AND APP_ENABLE_TSAN)
    message(FATAL_ERROR "ASan and TSan cannot be used simultaneously.")
endif()

if(APP_ENABLE_CCACHE AND APP_ENABLE_SCCACHE)
    message(FATAL_ERROR "APP_ENABLE_CCACHE and APP_ENABLE_SCCACHE cannot be enabled together.")
endif()

if(NOT APP_CXX_STANDARD MATCHES "^[0-9]+$")
    message(FATAL_ERROR "APP_CXX_STANDARD must be a numeric C++ standard value such as 20, 23, or 26.")
endif()

set(CMAKE_CXX_STANDARD "${APP_CXX_STANDARD}")
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---- INTERFACE targets (consumed by all project targets) ----
add_library(project_options INTERFACE)
add_library(project_warnings INTERFACE)

# ---- Warnings ----
if(MSVC)
    target_compile_options(project_warnings INTERFACE
        /W4
        /permissive-
        /Zc:__cplusplus
    )
else()
    target_compile_options(project_warnings INTERFACE
        -Wall -Wextra -Wpedantic
    )
endif()

if(APP_WARNINGS_AS_ERRORS)
    if(MSVC)
        target_compile_options(project_warnings INTERFACE /WX)
    else()
        target_compile_options(project_warnings INTERFACE -Werror)
    endif()
endif()

# ---- Compiler launchers ----
set(APP_COMPILER_LAUNCHER "")
if(APP_ENABLE_CCACHE)
    find_program(APP_COMPILER_LAUNCHER ccache)
    if(NOT APP_COMPILER_LAUNCHER)
        message(WARNING "[ccache] Requested but ccache not found")
    endif()
elseif(APP_ENABLE_SCCACHE)
    find_program(APP_COMPILER_LAUNCHER sccache)
    if(NOT APP_COMPILER_LAUNCHER)
        message(WARNING "[sccache] Requested but sccache not found")
    endif()
endif()

if(APP_COMPILER_LAUNCHER)
    set(CMAKE_CXX_COMPILER_LAUNCHER "${APP_COMPILER_LAUNCHER}" CACHE STRING "" FORCE)
    message(STATUS "[compiler-launcher] Enabled: ${APP_COMPILER_LAUNCHER}")
else()
    unset(CMAKE_CXX_COMPILER_LAUNCHER CACHE)
endif()

# ---- IPO/LTO ----
if(APP_ENABLE_IPO)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT APP_IPO_SUPPORTED OUTPUT APP_IPO_ERROR)
    if(APP_IPO_SUPPORTED)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
        set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL ON)
        message(STATUS "[ipo] Enabled for Release-style configurations")
    else()
        message(WARNING "[ipo] Requested but not supported: ${APP_IPO_ERROR}")
    endif()
endif()

# ---- clang-tidy (build-time) ----
if(APP_ENABLE_CLANG_TIDY)
    find_program(CLANG_TIDY_COMMAND clang-tidy)
    if(CLANG_TIDY_COMMAND)
        string(REGEX REPLACE "([][(){}.^$*+?|\\\\])" "\\\\\\1"
               APP_CLANG_TIDY_HEADER_FILTER "${CMAKE_SOURCE_DIR}/")
        set(CMAKE_CXX_CLANG_TIDY
            "${CLANG_TIDY_COMMAND}"
            "--config-file=${CMAKE_SOURCE_DIR}/.clang-tidy"
            "--header-filter=^${APP_CLANG_TIDY_HEADER_FILTER}"
        )
        message(STATUS "[clang-tidy] Enabled: ${CLANG_TIDY_COMMAND}")
    else()
        message(WARNING "[clang-tidy] Requested but clang-tidy not found")
    endif()
endif()

# ---- Sanitizers ----
include(Sanitizers)
