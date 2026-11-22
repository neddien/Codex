#!/usr/bin/env python3

import os
import re
import sys
import time
import json
import shutil
import ctypes
import platform
from enum import Enum
import subprocess as sb
from datetime import datetime
from typing import NoReturn


class Chrono:
    tp = None

    @staticmethod
    def begin():
        Chrono.tp = time.time()

    @staticmethod
    def end():
        if Chrono.tp is None:
            raise ValueError("Called Chrono.end() before calling Chrono.begin()")
        elapsed = time.time() - Chrono.tp
        tp = None
        return elapsed * 1000


class CMakeInspector:
    cache_file = None

    def __init__(self, build_dir):
        self.cache_file = os.path.join(build_dir, "CMakeCache.txt")
        if not os.path.exists(self.cache_file):
            raise RuntimeError("Invalid build directory.")

    def get_var(self, var_name):
        with open(self.cache_file, "r") as fs:
            for line in fs:
                if line.startswith(var_name):
                    return line.strip().split("=")[1]

class CMakePresetInspector:
    class Preset:
        json_ = None

        def __init__(self, json_obj):
            if json_obj:
                self.json_ = json_obj
            else:
                raise RuntimeError("Passed an invalid json object to Preset")

        def name(self) -> str:
            return json_["name"]
        def hidden(self) -> bool:
            return json_["hidden"]
        def generator(self) -> str:
            return json_["generator"]
        def binary_dir(self) -> str:
            return json_["binaryDir"]
        def install_dir(self) -> str:
            return json_["installDir"]
        def display_name(self) -> str:
            return json_["displayName"]
        def toolchain_file(self) -> str:
            return json_["toolchainFile"]
        def inherits(self) -> list[str]:
            if isinstance(json_["inherits"], str):
                return [json_["inherits"]]
            return json_["inherits"]

    preset_file_: str = None
    json_  = None

    def __init__(self, file_path):
        self.preset_file_ = file_path
        if not os.path.exists(self.preset_file_):
            raise RuntimeError("Invalid CMake Presets file.")

        self.json_ = json.loads(self.preset_file_)

    def presets(self) -> list[Preset]:
        preset_list: list[str] = []

        presets = self.json_["configurePresets"]
        if presets:
            for preset_json in presets:
                preset_list.append(Preset(preset_json))

        return preset_list


# ---------------------------------------------------------------------------
# Logging: same format and colours as the engine's console sink
# (codex_stdout_color_sink in codex/src/engine/core/log.cpp)
#
#   Nov 22 11:31:02 Build info: message
#
# Call set_tag() once at the top of a script to name it in the log lines.
# ---------------------------------------------------------------------------

_log_tag = "Script"
_DARK_GRAY = (169, 169, 169)
_DEEP_SKY_BLUE = (0, 191, 255)
_LEVEL_COLOURS = {
    "info": _DARK_GRAY,
    "warn": (255, 255, 0),
    "error": (255, 0, 0),
    "fatal": (139, 0, 0),
    "debug": (144, 238, 144),
    "verbose": (128, 128, 128),
}


def _colour_supported() -> bool:
    if not sys.stdout.isatty() or "NO_COLOR" in os.environ:
        return False
    if platform.system() != "Windows":
        return True
    try:  # Turn on ANSI escape handling for this console.
        from ctypes import wintypes
        kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
        kernel32.GetStdHandle.restype = wintypes.HANDLE
        kernel32.GetConsoleMode.argtypes = [wintypes.HANDLE, ctypes.POINTER(wintypes.DWORD)]
        kernel32.SetConsoleMode.argtypes = [wintypes.HANDLE, wintypes.DWORD]
        handle = kernel32.GetStdHandle(-11)  # STD_OUTPUT_HANDLE
        mode = wintypes.DWORD()
        if not kernel32.GetConsoleMode(handle, ctypes.byref(mode)):
            return False
        return bool(kernel32.SetConsoleMode(handle, mode.value | 0x0004))  # ENABLE_VIRTUAL_TERMINAL_PROCESSING
    except (AttributeError, OSError):
        return False


_COLOUR = _colour_supported()


def set_tag(tag: str) -> None:
    """Name shown after the timestamp, like the engine's logger tag."""
    global _log_tag
    _log_tag = tag


def _styled(text: str, rgb: tuple[int, int, int]) -> str:
    return f"\x1b[38;2;{rgb[0]};{rgb[1]};{rgb[2]}m{text}\x1b[0m" if _COLOUR else text


def _log(level: str, msg: str) -> None:
    now = datetime.now()
    colour = _LEVEL_COLOURS[level]
    print(f"{_styled(now.strftime('%b %d'), _DARK_GRAY)} {_styled(now.strftime('%H:%M:%S'), _DARK_GRAY)} "
          f"{_styled(_log_tag, _DEEP_SKY_BLUE)} {_styled(level, colour)}: {_styled(msg, colour)}", flush=True)


def verbose(msg: str) -> None:
    _log("verbose", msg)


def debug(msg: str) -> None:
    _log("debug", msg)


def info(msg: str) -> None:
    _log("info", msg)


def warn(msg: str) -> None:
    _log("warn", msg)


def error(msg: str) -> None:
    _log("error", msg)


def fatal(msg: str, exit_code: int = 1) -> NoReturn:
    """Log at the fatal level and exit the script."""
    _log("fatal", msg)
    sys.exit(exit_code)


def run(cmd, shell=True, stdout=None, stderr=None, capture_output=False, text=None):
    return sb.run(
        cmd,
        shell=shell,
        stdout=stdout,
        stderr=stderr,
        capture_output=capture_output,
        text=text,
    )


def get_cmake_presets():
    res = run("cmake --list-presets", shell=True, stdout=sb.PIPE)
    if res:
        output = res.stdout.decode("utf-8")
        if output == "":
            return []
    else:
        return []

    # Pythonic autism
    presets = output.replace(" ", "").split("\n")
    presets = presets[2:-1]
    presets = [(s[: s.rfind('"')])[1:] for s in presets]

    return presets


def win32_is_admin():
    try:
        return ctypes.windll.shell32.IsUserAdmin()
    except:
        return False


def win32_request_admin():
    ctypes.windll.shell32.ShellExecuteW(
        None, "runas", sys.executable, " ".join(sys.argv), None, 1
    )
