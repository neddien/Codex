#!/usr/bin/env python3

import os
import re
import sys
import time
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
