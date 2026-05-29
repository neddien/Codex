import subprocess
import os
from conan import ConanFile
from conan.tools.cmake import CMakeToolchain


class CodexConan(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    generators = "CMakeToolchain", "CMakeDeps"

    def generate(self):
        tc = CMakeToolchain(self)
        tc.user_presets_path = False

    def requirements(self):
        self.requires("sdl/2.32.10")
        self.requires("box2d/2.4.2")
        self.requires("nlohmann_json/3.12.0")
        self.requires("lz4/1.10.0")
        self.requires("imgui/1.90.5-docking")
        self.requires("fmt/12.1.0")
        self.requires("spdlog/1.17.0")
        self.requires("entt/3.13.2")
        self.requires("magic_enum/0.9.7")
        self.requires("glm/1.0.1")
        self.requires("stb/cci.20240531")
        self.requires("abseil/20260107.1")
        self.requires("cxxopts/3.3.1")
        #self.requires("glad/2.0.8")

    def configure(self):
        self.options["fmt"].shared = True
        self.options["sdl"].shared = True
        self.options["glm"].shared = False
        self.options["magic_enum"].shared = False
        self.options["stb"].shared = False
        #self.options["box2d"].shared = True
        #self.options["nlohmann_json"].shared = True
        #self.options["lz4"].shared = True

        # Audio handled by FMOD; disable SDL's audio backends to avoid pulling
        # in PulseAudio, ALSA, libsndfile, and the rest of that stack.
        self.options["sdl"].pulse = False
        self.options["sdl"].alsa  = False
        if self.settings.os == "Linux":
            if os.environ["XDG_SESSION_TYPE"] == "wayland":
                self.options["sdl"].wayland = True
            else:
                self.options["sdl"].wayland = False
