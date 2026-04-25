# cmake/ConanSetup.cmake
# Automatic Conan dependency installation during CMake configure.
#
# Include AFTER project() but BEFORE any find_package() calls for Conan deps.
# Runs `conan install` when dependency-relevant inputs change.

include_guard(GLOBAL)

macro(app_setup_conan)
    option(APP_AUTO_CONAN "Automatically run conan install during configure" ON)
    set(APP_CONAN_PROFILE_HOST "" CACHE STRING "Conan host profile passed to conan install")
    set(APP_CONAN_PROFILE_BUILD "" CACHE STRING "Conan build profile passed to conan install")
    set(APP_CONAN_INSTALL_ARGS "" CACHE STRING "Additional arguments appended to conan install")

    set(_conan_file "${CMAKE_SOURCE_DIR}/conanfile.py")
    if(NOT EXISTS "${_conan_file}")
        message(FATAL_ERROR "[Conan] conanfile.py not found at: ${_conan_file}")
    endif()

    find_program(CONAN_COMMAND conan)
    if(NOT CONAN_COMMAND)
        message(FATAL_ERROR "[Conan] conan not found. Install it: pip install conan")
    endif()

    set(_conan_output_dir "${CMAKE_BINARY_DIR}/conan")
    if(CMAKE_CXX_STANDARD)
        set(_conan_cpp_standard "${CMAKE_CXX_STANDARD}")
    elseif(APP_CXX_STANDARD)
        set(_conan_cpp_standard "${APP_CXX_STANDARD}")
    else()
        set(_conan_cpp_standard "23")
    endif()

    # Determine build type without mutating CMAKE_BUILD_TYPE.
    if(CMAKE_CONFIGURATION_TYPES)
        if(CMAKE_DEFAULT_BUILD_TYPE)
            set(_conan_build_type "${CMAKE_DEFAULT_BUILD_TYPE}")
        else()
            set(_conan_build_type "Debug")
        endif()
    elseif(CMAKE_BUILD_TYPE)
        set(_conan_build_type "${CMAKE_BUILD_TYPE}")
    else()
        set(_conan_build_type "Debug")
    endif()

    set(_conan_generated_profile_host "")
    if(WIN32 AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT APP_CONAN_PROFILE_HOST)
        string(REGEX MATCH "^[0-9]+" _conan_gcc_major "${CMAKE_CXX_COMPILER_VERSION}")
        if(NOT _conan_gcc_major)
            message(FATAL_ERROR "[Conan] Could not determine GCC major version from: ${CMAKE_CXX_COMPILER_VERSION}")
        endif()

        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            set(_conan_arch "x86_64")
        else()
            set(_conan_arch "x86")
        endif()

        set(_conan_generated_profile_host "${_conan_output_dir}/profiles/mingw-host")
        file(MAKE_DIRECTORY "${_conan_output_dir}/profiles")
        file(WRITE "${_conan_generated_profile_host}"
            "[settings]\n"
            "os=Windows\n"
            "arch=${_conan_arch}\n"
            "compiler=gcc\n"
            "compiler.version=${_conan_gcc_major}\n"
            "compiler.libcxx=libstdc++11\n"
            "compiler.threads=posix\n"
            "compiler.exception=seh\n"
            "compiler.cppstd=${_conan_cpp_standard}\n"
            "build_type=${_conan_build_type}\n"
        )
    endif()

    set(_conan_lockfile "${CMAKE_SOURCE_DIR}/conan.lock")
    if(EXISTS "${_conan_lockfile}")
        file(SHA256 "${_conan_lockfile}" _conan_lock_hash)
    else()
        set(_conan_lock_hash "no-lockfile")
    endif()

    execute_process(
        COMMAND "${CONAN_COMMAND}" --version
        OUTPUT_VARIABLE _conan_version
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
    )

    set(_conan_fingerprint_inputs
        "conan_version=${_conan_version}"
        "build_type=${_conan_build_type}"
        "generator=${CMAKE_GENERATOR}"
        "system_name=${CMAKE_SYSTEM_NAME}"
        "system_processor=${CMAKE_SYSTEM_PROCESSOR}"
        "compiler_id=${CMAKE_CXX_COMPILER_ID}"
        "compiler_version=${CMAKE_CXX_COMPILER_VERSION}"
        "cpp_standard=${_conan_cpp_standard}"
        "profile_host=${APP_CONAN_PROFILE_HOST}"
        "generated_profile_host=${_conan_generated_profile_host}"
        "profile_build=${APP_CONAN_PROFILE_BUILD}"
        "install_args=${APP_CONAN_INSTALL_ARGS}"
        "conan_home=$ENV{CONAN_HOME}"
        "conan_profile=$ENV{CONAN_PROFILE}"
        "conan_profile_host=$ENV{CONAN_PROFILE_HOST}"
        "conan_profile_build=$ENV{CONAN_PROFILE_BUILD}"
    )

    string(JOIN "\n" _conan_fingerprint "${_conan_fingerprint_inputs}")

    # Re-run conan install when dependency inputs change.
    file(SHA256 "${_conan_file}" _conan_file_hash)
    string(SHA256 _conan_input_hash "${_conan_fingerprint}\nconanfile=${_conan_file_hash}\nlockfile=${_conan_lock_hash}")
    set(_conan_stamp "${_conan_output_dir}/.stamp_${_conan_build_type}_${_conan_input_hash}")

    if(APP_AUTO_CONAN AND NOT EXISTS "${_conan_stamp}")
        set(_conan_command
            "${CONAN_COMMAND}" install "${CMAKE_SOURCE_DIR}"
            --output-folder "${_conan_output_dir}"
            -s "build_type=${_conan_build_type}"
            -s "compiler.cppstd=${_conan_cpp_standard}"
            --build=missing
        )

        if(APP_CONAN_PROFILE_HOST)
            list(APPEND _conan_command --profile:host "${APP_CONAN_PROFILE_HOST}")
        elseif(_conan_generated_profile_host)
            list(APPEND _conan_command --profile:host "${_conan_generated_profile_host}")
        endif()

        if(APP_CONAN_PROFILE_BUILD)
            list(APPEND _conan_command --profile:build "${APP_CONAN_PROFILE_BUILD}")
        endif()

        if(APP_CONAN_INSTALL_ARGS)
            separate_arguments(_conan_extra_args NATIVE_COMMAND "${APP_CONAN_INSTALL_ARGS}")
            list(APPEND _conan_command ${_conan_extra_args})
        endif()

        message(STATUS "[Conan] Installing dependencies (build_type=${_conan_build_type}) ...")

        execute_process(
            COMMAND ${_conan_command}
            RESULT_VARIABLE _conan_result
            WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        )

        if(NOT _conan_result EQUAL 0)
            message(FATAL_ERROR "[Conan] install failed (exit code: ${_conan_result})")
        endif()

        # Clear old stamps, write new one
        file(GLOB _old_stamps "${_conan_output_dir}/.stamp_*")
        if(_old_stamps)
            file(REMOVE ${_old_stamps})
        endif()
        file(TOUCH "${_conan_stamp}")
    elseif(APP_AUTO_CONAN)
        message(STATUS "[Conan] Dependencies up to date (cached)")
    else()
        message(STATUS "[Conan] APP_AUTO_CONAN=OFF; expecting dependencies in ${_conan_output_dir}")
    endif()

    # Make Conan-generated find-modules available to find_package()
    list(PREPEND CMAKE_PREFIX_PATH "${_conan_output_dir}")
    list(PREPEND CMAKE_MODULE_PATH "${_conan_output_dir}")
endmacro()
