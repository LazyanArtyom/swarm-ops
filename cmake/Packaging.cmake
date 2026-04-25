# cmake/Packaging.cmake
include_guard(GLOBAL)

function(app_packaging_generate)
  # Expects (from top-level):
  # APP_SLUG APP_ID APP_VENDOR APP_DISPLAY_NAME APP_VERSION APP_RELEASE_DATE
  # QTIFW_ROOT (optional), APP_DIST_DIR, CMAKE_SYSTEM_PROCESSOR

  # ------------------------------
  # Version text (used by scripts)
  # ------------------------------
  configure_file(
    "${CMAKE_SOURCE_DIR}/packaging/version.txt.in"
    "${CMAKE_BINARY_DIR}/version.txt"
    @ONLY
  )

  string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" APP_PACKAGE_ARCH_RAW)
  if(APP_PACKAGE_ARCH_RAW MATCHES "^(x86_64|amd64)$")
    set(APP_PACKAGE_ARCH "x86_64")
  elseif(APP_PACKAGE_ARCH_RAW MATCHES "^(aarch64|arm64)$")
    set(APP_PACKAGE_ARCH "aarch64")
  elseif(APP_PACKAGE_ARCH_RAW MATCHES "^(x86|i[3-6]86)$")
    set(APP_PACKAGE_ARCH "x86")
  elseif(APP_PACKAGE_ARCH_RAW MATCHES "^(armv7l|armv7|armhf)$")
    set(APP_PACKAGE_ARCH "armv7")
  else()
    set(APP_PACKAGE_ARCH "${APP_PACKAGE_ARCH_RAW}")
  endif()

  file(MAKE_DIRECTORY "${APP_DIST_DIR}")

  set(_qtifw_binarycreator_hints)

  if(QTIFW_ROOT)
    list(APPEND _qtifw_binarycreator_hints "${QTIFW_ROOT}/bin")
  endif()

  if(DEFINED ENV{QTIFW_ROOT} AND NOT "$ENV{QTIFW_ROOT}" STREQUAL "")
    list(APPEND _qtifw_binarycreator_hints "$ENV{QTIFW_ROOT}/bin")
  endif()

  if(DEFINED ENV{HOME} AND NOT "$ENV{HOME}" STREQUAL "")
    file(GLOB _qtifw_home_roots
      LIST_DIRECTORIES true
      "$ENV{HOME}/Qt/Tools/QtInstallerFramework/*"
    )
    list(SORT _qtifw_home_roots COMPARE NATURAL ORDER DESCENDING)
    foreach(_qtifw_home_root IN LISTS _qtifw_home_roots)
      list(APPEND _qtifw_binarycreator_hints "${_qtifw_home_root}/bin")
    endforeach()
  endif()

  find_program(APP_BINARYCREATOR_EXECUTABLE
    NAMES binarycreator binarycreator.exe
    HINTS ${_qtifw_binarycreator_hints}
  )

  # ------------------------------
  # IFW config generation (Linux/Windows)
  # ------------------------------
  set(QTIFW_OUT_DIR    "${CMAKE_BINARY_DIR}/qtifw")
  set(QTIFW_CONFIG_DIR "${QTIFW_OUT_DIR}/config")

  set(IFW_PACKAGE_ID "${APP_ID}")
  set(QTIFW_PKG_DIR  "${QTIFW_OUT_DIR}/packages/${IFW_PACKAGE_ID}")
  set(QTIFW_PKG_META "${QTIFW_PKG_DIR}/meta")
  set(IFW_INSTALLER_ICON_XML "")

  if(NOT APPLE)
    file(MAKE_DIRECTORY "${QTIFW_CONFIG_DIR}" "${QTIFW_PKG_META}")

    # Preserve IFW @Variables@ literally when configuring
    set(ApplicationsDir "@ApplicationsDir@")
    set(TargetDir       "@TargetDir@")
    set(StartMenuDir    "@StartMenuDir@")
    set(DesktopDir      "@DesktopDir@")

    if(WIN32 AND EXISTS "${CMAKE_SOURCE_DIR}/packaging/icons/app_icon_win.ico")
      configure_file(
        "${CMAKE_SOURCE_DIR}/packaging/icons/app_icon_win.ico"
        "${QTIFW_CONFIG_DIR}/${APP_SLUG}.ico"
        COPYONLY
      )
      set(IFW_INSTALLER_ICON_XML "    <InstallerApplicationIcon>${APP_SLUG}</InstallerApplicationIcon>")
    endif()

    configure_file(
      "${CMAKE_SOURCE_DIR}/packaging/ifw/config/config.xml.in"
      "${QTIFW_CONFIG_DIR}/config.xml"
      @ONLY
    )
    configure_file(
      "${CMAKE_SOURCE_DIR}/packaging/ifw/config/controller.qs.in"
      "${QTIFW_CONFIG_DIR}/controller.qs"
      @ONLY
    )

    configure_file(
      "${CMAKE_SOURCE_DIR}/packaging/ifw/packages/package.xml.in"
      "${QTIFW_PKG_META}/package.xml"
      @ONLY
    )
    configure_file(
      "${CMAKE_SOURCE_DIR}/packaging/ifw/packages/installscript.qs.in"
      "${QTIFW_PKG_META}/installscript.qs"
      @ONLY
    )
  endif()

  # ------------------------------
  # Platform-specific extras
  # ------------------------------

  # Linux: .desktop generation
  if(UNIX AND NOT APPLE)
    file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/packaging/linux")
    configure_file(
      "${CMAKE_SOURCE_DIR}/packaging/linux/app.desktop.in"
      "${CMAKE_BINARY_DIR}/packaging/linux/${APP_SLUG}.desktop"
      @ONLY
    )
  endif()

  # ------------------------------
  # Export packaging variables into build dir (scripts read it)
  # (common: all OS)
  # ------------------------------
  set(PACKAGING_OUT_DIR "${CMAKE_BINARY_DIR}/packaging")
  file(MAKE_DIRECTORY "${PACKAGING_OUT_DIR}")

  configure_file(
    "${CMAKE_SOURCE_DIR}/packaging/scripts/packaging.env.sh.in"
    "${PACKAGING_OUT_DIR}/packaging.env.sh"
    @ONLY
  )
  configure_file(
    "${CMAKE_SOURCE_DIR}/packaging/scripts/packaging.env.bat.in"
    "${PACKAGING_OUT_DIR}/packaging.env.bat"
    @ONLY
  )

  # Export key dirs if scripts/tools want them
  set(APP_PACKAGE_ARCH "${APP_PACKAGE_ARCH}" PARENT_SCOPE)
  set(QTIFW_OUT_DIR "${QTIFW_OUT_DIR}" PARENT_SCOPE)
  set(QTIFW_CONFIG_DIR "${QTIFW_CONFIG_DIR}" PARENT_SCOPE)
  set(PACKAGING_OUT_DIR "${PACKAGING_OUT_DIR}" PARENT_SCOPE)
endfunction()
