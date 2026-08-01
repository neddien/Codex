import subprocess
import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain

class NBConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def generate(self):
        tc = CMakeToolchain(self)
        tc.user_presets_path = False

    def requirements(self):
        self.requires("fmt/12.1.0")
        self.requires("entt/3.13.2")
        self.requires("magic_enum/0.9.7")
        self.requires("glm/1.0.1")
        self.requires("abseil/20260107.1")
        self.requires("enet/1.3.18")

    def configure(self):
        self.options["fmt"].shared = True
        self.options["glm"].shared = False
        self.options["magic_enum"].shared = False
