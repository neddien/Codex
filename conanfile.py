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
        self.requires("imgui/1.92.7-docking")

    def configure(self):
        self.options["sdl"].shared = False
        # Audio handled by FMOD; disable SDL's audio backends to avoid pulling
        # in PulseAudio, ALSA, libsndfile, and the rest of that stack.
        self.options["sdl"].pulse = False
        self.options["sdl"].alsa  = False
        if self.settings.os == "Linux":
            if os.environ["XDG_SESSION_TYPE"] == "wayland":
                self.options["sdl"].wayland = True
            else:
                self.options["sdl"].wayland = False
