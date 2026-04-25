# SwarmOps

Cross-platform Qt/CMake desktop client for the SwarmOps drone swarm platform, with:
- CMake presets
- Conan dependency management
- Qt Test integration
- clang-tidy tasks
- cross-platform packaging hooks for Linux, Windows, and macOS

The client is the GUI entry point described in the SwarmOps architecture: it will
communicate with the platform through the public API Gateway contract, while
backend services own SwarmKit integration, telemetry ingestion, mission
workspace persistence, and collaboration.

## Requirements

### Common
- CMake 3.28+
- Git
- Ninja recommended
- Conan 2.x
- A C++23-capable compiler

### Linux
- GCC or Clang
- Qt 6.8.3 LTS for desktop Linux (`gcc_64`)
- Qt 5 Compatibility Module
- Qt Installer Framework if you want to build Linux installers

Fresh Ubuntu 24.04 setup:

```bash
sudo apt-get update
sudo apt-get install -y build-essential git pkg-config cmake ninja-build libgl1-mesa-dev libxcb-cursor0 libgtk-3-0t64 pipx
pipx ensurepath
```

After `pipx ensurepath`, restart your terminal or reload your login shell:

```bash
exec "$SHELL" -l
```

Install Conan:

```bash
pipx install conan
conan --version
conan profile detect --force
```

Qt installer / Maintenance Tool selections:
- Install `Qt 6.8.3 LTS` with the `Desktop gcc_64` kit.
- Install `Qt 5 Compatibility Module`.
- Install `Qt Installer Framework` if you want to build Linux installers.
- In the installer tool, also select `CMake` and `Ninja`, or use the Ubuntu packages from the command above.

The Linux preset expects Qt at:

```text
$HOME/Qt/6.8.3/gcc_64
```

If your Qt install lives somewhere else, update `QT_ROOT` in `CMakePresets.json` or pass `-DQT_ROOT=/path/to/Qt/6.8.3/gcc_64` during configure.

### Windows
- Windows 11
- MSYS2 with the `MINGW64` environment
- Qt 6.8.3 with the MinGW 64-bit kit
- Qt 5 Compatibility Module
- Qt Installer Framework if you want to build Windows installers
- PowerShell for system setup tasks
- OpenSSH / remote access optional for CI agents

Use the **MSYS2 MinGW 64-bit** shell for all configure/build/package commands. Do not use the plain MSYS shell for this project.

Fresh Windows setup for this project:

1. Install **MSYS2**.
2. Open the **MSYS2 MinGW 64-bit** shell.
3. Update MSYS2:
   ```bash
   pacman -Syu
   ```
   If prompted, close the shell, reopen **MSYS2 MinGW 64-bit**, then run:
   ```bash
   pacman -Syu
   ```
4. Install required build tools:
   ```bash
   pacman -S --needed \
     mingw-w64-x86_64-toolchain \
     mingw-w64-x86_64-cmake \
     mingw-w64-x86_64-ninja \
     mingw-w64-x86_64-pkgconf \
     mingw-w64-x86_64-python-pip \
   ```
5. Install Qt with the Qt online installer / Maintenance Tool:
   - `Qt 6.8.3` with the `mingw_64` kit
   - `Qt 5 Compatibility Module`
   - `Qt Installer Framework` if you want Windows installers
6. Create and activate a Python virtual environment for Conan from the **MINGW64** shell:
   ```bash
   python -m venv ~/.venvs/conan2
   source ~/.venvs/conan2/bin/activate
   ```
7. Install Conan into that virtual environment:
   ```bash
   python -m pip install --upgrade pip
   python -m pip install conan
   ```
8. Detect the default Conan profile:
   ```bash
   conan profile detect --force
   ```
9. Verify the main tools:
   ```bash
   echo $MSYSTEM
   g++ --version
   cmake --version
   ninja --version
   conan --version
   ```
   `echo $MSYSTEM` must print:
   ```text
   MINGW64
   ```

Qt installer / Maintenance Tool selections:
- Install `Qt 6.8.3` with the `Desktop MinGW 64-bit` kit.
- Install `Qt 5 Compatibility Module`.
- Install `Qt Installer Framework` if you want to build Windows installers.

The Windows presets expect Qt tools compatible with the MinGW kit selected in `CMakePresets.json`. If your Qt install lives somewhere else, update the preset variables or pass the needed cache variables during configure.

Qt Installer Framework is resolved during CMake configure from `QTIFW_ROOT` first and then from `PATH`, and the resolved `binarycreator` path is exported to the packaging scripts.

#### Windows notes
- Run `cmake`, `ctest`, and packaging commands from the **MSYS2 MinGW 64-bit** shell.
- If Conan is installed in a virtual environment, activate it before running `cmake --preset ...`.
- If your shell prompt does not show the venv name, Conan may not be on `PATH`.
- For CI, explicitly start the `mingw64` shell instead of relying on the global Windows `PATH`.

Example shell startup from `cmd.exe` / Jenkins:

```cmd
call "C:\msys64\msys2_shell.cmd" -defterm -no-start -mingw64 -here
```

Example configure/build from the MSYS2 `MINGW64` shell:

```bash
source ~/.venvs/conan2/bin/activate
cmake --preset win-release
cmake --build --preset win-release
ctest --preset win-release
```

### macOS
- Xcode Command Line Tools / Apple Clang
- Qt 6.x

## Daily Commands

### Debug build

Linux:
```bash
cmake --preset linux-debug
cmake --build --preset linux-debug
```

Windows (run from **MSYS2 MinGW 64-bit** shell):
```bash
source ~/.venvs/conan2/bin/activate
cmake --preset win-debug
cmake --build --preset win-debug
```

macOS:
```bash
cmake --preset mac-debug
cmake --build --preset mac-debug
```

### Run tests

Linux:
```bash
ctest --preset linux-debug
```

Windows (run from **MSYS2 MinGW 64-bit** shell):
```bash
source ~/.venvs/conan2/bin/activate
ctest --preset win-debug
```

macOS:
```bash
ctest --preset mac-debug
```

### Release build

Linux:
```bash
cmake --preset linux-release
cmake --build --preset linux-release
ctest --preset linux-release
```

Windows (run from **MSYS2 MinGW 64-bit** shell):
```bash
source ~/.venvs/conan2/bin/activate
cmake --preset win-release
cmake --build --preset win-release
ctest --preset win-release
```

macOS:
```bash
cmake --preset mac-release
cmake --build --preset mac-release
ctest --preset mac-release
```

## VS Code Task Flow

This repo uses `.vscode/tasks.json` as the main local workflow layer.

Recommended user keybindings:

```json
[
  { "key": "f5", "command": "workbench.action.debug.start" },
  { "key": "f6", "command": "workbench.action.tasks.runTask", "args": "kb: build debug" },
  { "key": "shift+f6", "command": "workbench.action.tasks.runTask", "args": "kb: build release" },
  { "key": "f7", "command": "workbench.action.tasks.runTask", "args": "kb: run debug" },
  { "key": "shift+f7", "command": "workbench.action.tasks.runTask", "args": "kb: run release" },
  { "key": "f8", "command": "workbench.action.tasks.runTask", "args": "kb: deploy installer" }
]
```

Key meanings:

| Key | Action |
| --- | --- |
| `F5` | Debug with debugger |
| `F6` | Configure/build debug and refresh `compile_commands.json` |
| `Shift+F6` | Configure/build release and refresh `compile_commands.json` |
| `F7` | Run debug app |
| `Shift+F7` | Run release app |
| `F8` | Configure release, build release, run release tests, then create installer |

On macOS you may need `Fn + Fx` depending on system keyboard settings.

## Testing

The SwarmOps client includes:
- `CTest`
- `Qt Test`
- focused tests under `tests/`
- coverage for configuration, logging, theme, and shell panel foundations

Add new test files under `tests/` and register them in [tests/CMakeLists.txt](tests/CMakeLists.txt).

## Packaging

Direct packaging commands:

Linux:
```bash
packaging/scripts/make_installer_linux.sh build/linux-release
```

Windows (run from **MSYS2 MinGW 64-bit** shell):
```bash
packaging/scripts/make_installer_win.bat "build/win-release" "%CD%"
```

macOS:
```bash
packaging/scripts/make_installer_mac.sh build/mac-release
```

Final installers are copied into `dist/`.

Using VS Code `F8` is preferred because it runs the full release chain first.

Packaging strategy by platform:
- Linux: Qt deploy script + Qt IFW offline installer built from an installed staging tree
- Windows: Qt IFW offline installer built from an installed staging tree and `windeployqt`
- macOS: native `.app` bundle + DMG built with `macdeployqt` and `hdiutil`

`APP_PACKAGE_ARCH` is generated by CMake from the active target architecture and reused by every packaging script, so installer names stay consistent across platforms.

## Conan Notes

Conan is integrated into configure. By default:
- `cmake --preset ...` runs `conan install`
- generated Conan files stay inside the build tree
- deleting `build/` is safe and does not leave broken source-root preset files behind

If you want manual/offline/CI control, configure with:

```bash
cmake --preset mac-debug -DAPP_AUTO_CONAN=OFF
```

Then you are responsible for running `conan install` yourself before configure/build.

On Windows with MSYS2:
- keep Conan in a Python virtual environment
- activate the venv before configure/build
- for CI, activate the venv in the same `MINGW64` shell session that runs CMake

Example manual Windows Conan flow:

```bash
source ~/.venvs/conan2/bin/activate
conan install . --build=missing
cmake --preset win-debug -DAPP_AUTO_CONAN=OFF
cmake --build --preset win-debug
```

## Useful Build Knobs

These can be appended to any preset command with `-D...=ON`.

`-DAPP_ENABLE_IPO=ON`
- Enables interprocedural optimization / link-time optimization for release-style builds.
- Useful for production binaries when toolchain support is available.

`-DAPP_WARNINGS_AS_ERRORS=ON`
- Treats compiler warnings as build errors.
- Good for CI and for keeping new projects strict from day one.

`-DAPP_ENABLE_CCACHE=ON`
- Uses `ccache` as the compiler launcher if installed.
- Speeds up repeated local builds on Linux/macOS and some toolchains.

`-DAPP_ENABLE_SCCACHE=ON`
- Uses `sccache` as the compiler launcher if installed.
- Useful when you want local caching or shared/remote cache workflows.

`-DAPP_AUTO_CONAN=OFF`
- Disables automatic `conan install` during configure.
- Useful in CI, offline development, or when dependency resolution should be explicit.

`-DAPP_CXX_STANDARD=23`
- Sets the project C++ standard in one place for both CMake targets and Conan dependency resolution.
- Default is `23` today, and you can move to `26` later without editing multiple files.

`-DAPP_DIST_DIR=/abs/path/to/dist`
- Chooses where final installers are copied.
- Default is the repo-local `dist/` folder.

Examples:

```bash
cmake --preset mac-release -DAPP_ENABLE_IPO=ON -DAPP_WARNINGS_AS_ERRORS=ON
cmake --preset mac-debug -DAPP_ENABLE_CCACHE=ON
cmake --preset mac-debug -DAPP_AUTO_CONAN=OFF
cmake --preset mac-debug -DAPP_CXX_STANDARD=26
```

Windows examples from the **MINGW64** shell:

```bash
source ~/.venvs/conan2/bin/activate
cmake --preset win-release -DAPP_ENABLE_IPO=ON -DAPP_WARNINGS_AS_ERRORS=ON
cmake --preset win-debug -DAPP_AUTO_CONAN=OFF
cmake --preset win-debug -DAPP_CXX_STANDARD=26
```

## VS Code Font

The workspace uses JetBrains Mono. If source code looks dimmed, washed out, or oddly rendered after opening the project in VS Code, install the font and reload VS Code:

```bash
sudo apt-get update
sudo apt-get install -y fonts-jetbrains-mono
fc-cache -f
```

Then run `Developer: Reload Window` from the VS Code command palette.

## Recommended VS Code Extensions

- `ms-vscode.cpptools`
- `llvm-vs-code-extensions.vscode-clangd`
- `ms-vscode.cmake-tools`
- `twxs.cmake`
- `qwtel.sqlite-viewer` if SwarmOps later adds local SQLite inspection tools
- `Gruntfuggly.todo-tree`
