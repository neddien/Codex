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


def log(msg):
    print(f'[{datetime.now().strftime("%Y-%m-%d %H:%M:%S")}] [Info] (Build) :: {msg}')


def warn(msg):
    print(
        f'[{datetime.now().strftime("%Y-%m-%d %H:%M:%S")}] [Warning] (Build) :: {msg}'
    )


def err(msg):
    print(f'[{datetime.now().strftime("%Y-%m-%d %H:%M:%S")}] [Panic] (Build) :: {msg}')


def panic(msg, exit_code=1):
    err(msg)
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
