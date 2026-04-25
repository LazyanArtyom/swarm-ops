from conan import ConanFile
from conan.tools.cmake import CMakeDeps, CMakeToolchain


class SwarmOpsConan(ConanFile):
    settings = "os", "arch", "compiler", "build_type"

    def requirements(self):
        self.requires("spdlog/1.15.1")

    def generate(self):
        deps = CMakeDeps(self)
        deps.generate()

        toolchain = CMakeToolchain(self)
        # Keep Conan files inside the build tree so deleting build/ never breaks source-root presets.
        toolchain.user_presets_path = False
        toolchain.generate()
