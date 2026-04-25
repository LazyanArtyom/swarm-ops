# cmake/Sanitizers.cmake
# Per-target sanitizer support (ASan, TSan, UBSan).
# Modeled after swarmkit/cmake/SwarmkitSanitizers.cmake.

include_guard(GLOBAL)

function(app_enable_sanitizers target_name)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang|AppleClang|GNU")
        return()
    endif()

    if(APP_ENABLE_ASAN)
        target_compile_options(${target_name} PRIVATE -fsanitize=address -fno-omit-frame-pointer)
        target_link_options(${target_name} PRIVATE -fsanitize=address)
    endif()

    if(APP_ENABLE_TSAN)
        target_compile_options(${target_name} PRIVATE -fsanitize=thread -fno-omit-frame-pointer)
        target_link_options(${target_name} PRIVATE -fsanitize=thread)
    endif()

    if(APP_ENABLE_UBSAN)
        target_compile_options(${target_name} PRIVATE -fsanitize=undefined -fno-omit-frame-pointer)
        target_link_options(${target_name} PRIVATE -fsanitize=undefined)
    endif()
endfunction()
