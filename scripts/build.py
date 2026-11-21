#!/usr/bin/env python3
"""Codex build helper.

One command to configure, build, install and run Codex on any host, driven by
CMakePresets.json. Configuring a preset also installs its Conan packages through
the cmake-conan provider (cmake/conan-provider).

Run `python scripts/build.py -h` for the options and examples.
"""

import os
import re
import sys
import json
import time
import shutil
import platform
import argparse
import functools
import subprocess
from datetime import datetime
from pathlib import Path
from typing import NoReturn


ROOT = Path(__file__).resolve().parent.parent
HOST = platform.system()
EXE_SUFFIX = ".exe" if HOST == "Windows" else ""

DEFAULT_CONFIGS = ["Debug", "Release", "Shipping"]
# Preset used when none is given and no build directory has been configured yet.
DEFAULT_PRESET = {"Windows": "windows-vs2022", "Linux": "linux-gcc"}

EDITOR = "CodexEditor"
# Name prefix of the launch.json entries this script owns (other entries are kept).
LAUNCH_PREFIX = "(LLDB) Codex Editor"


# ---------------------------------------------------------------------------
# Logging: same format and colours as the engine's console sink
# (codex_stdout_color_sink in codex/src/engine/core/log.cpp)
# ---------------------------------------------------------------------------

LOG_TAG = "Build"
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
    if HOST != "Windows":
        return True
    try:  # Turn on ANSI escape handling for this console.
        import ctypes
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


def _styled(text: str, rgb: tuple[int, int, int]) -> str:
    return f"\x1b[38;2;{rgb[0]};{rgb[1]};{rgb[2]}m{text}\x1b[0m" if _COLOUR else text


def _log(level: str, msg: str) -> None:
    now = datetime.now()
    colour = _LEVEL_COLOURS[level]
    print(f"{_styled(now.strftime('%b %d'), _DARK_GRAY)} {_styled(now.strftime('%H:%M:%S'), _DARK_GRAY)} "
          f"{_styled(LOG_TAG, _DEEP_SKY_BLUE)} {_styled(level, colour)}: {_styled(msg, colour)}", flush=True)


def info(msg: str) -> None:
    _log("info", msg)


def warn(msg: str) -> None:
    _log("warn", msg)


def fatal(msg: str, exit_code: int = 1) -> NoReturn:
    _log("error", msg)
    sys.exit(exit_code)


# ---------------------------------------------------------------------------
# Presets
# ---------------------------------------------------------------------------

@functools.cache
def _preset_files() -> dict[str, dict]:
    """Configure presets from CMakePresets.json and CMakeUserPresets.json, by name."""
    presets: dict[str, dict] = {}
    for file_name in ("CMakePresets.json", "CMakeUserPresets.json"):
        path = ROOT / file_name
        if path.is_file():
            with open(path, encoding="utf-8") as fh:
                for preset in json.load(fh).get("configurePresets", []):
                    presets[preset["name"]] = preset
    return presets


def _preset_lineage(name: str) -> list[dict]:
    """The preset followed by everything it inherits, nearest first."""
    presets = _preset_files()
    order: list[str] = []

    def walk(current: str) -> None:
        if current in order or current not in presets:
            return
        order.append(current)
        inherits = presets[current].get("inherits", [])
        for parent in [inherits] if isinstance(inherits, str) else inherits:
            walk(parent)

    walk(name)
    return [presets[n] for n in order]


def _preset_field(name: str, field: str):
    for preset in _preset_lineage(name):
        if field in preset:
            return preset[field]
    return None


def _preset_cache_var(name: str, var: str) -> str | None:
    for preset in _preset_lineage(name):
        variables = preset.get("cacheVariables", {})
        if var in variables:
            value = variables[var]  # null in the preset means "unset"
            return value.get("value") if isinstance(value, dict) else value
    return None


@functools.cache
def _host_presets() -> list[str]:
    """Configure presets CMake accepts on this host (conditions already applied)."""
    try:
        res = subprocess.run(["cmake", "--list-presets"], cwd=ROOT, capture_output=True, text=True)
    except FileNotFoundError:
        fatal("cmake was not found on PATH.")
    if res.returncode != 0:
        fatal(f"cmake --list-presets failed:\n{res.stderr.strip()}")
    return re.findall(r'^\s+"([^"]+)"', res.stdout, re.MULTILINE)


def _build_dir(preset: str) -> Path:
    binary_dir = _preset_field(preset, "binaryDir")
    if binary_dir:
        expanded = binary_dir.replace("${sourceDir}", str(ROOT)).replace("${presetName}", preset)
        if "${" not in expanded:
            path = Path(expanded)
            return path if path.is_absolute() else ROOT / path
    return ROOT / "builds" / preset


# ---------------------------------------------------------------------------
# Build directory state
# ---------------------------------------------------------------------------

def _cache_var(build_dir: Path, name: str) -> str | None:
    try:
        with open(build_dir / "CMakeCache.txt", encoding="utf-8", errors="replace") as fh:
            for line in fh:
                key, sep, value = line.rstrip("\r\n").partition("=")
                if sep and key.split(":", 1)[0] == name:
                    return value
    except OSError:
        pass
    return None


def _is_configured(build_dir: Path) -> bool:
    if not (build_dir / "CMakeCache.txt").is_file():
        return False
    # A configure that failed half-way leaves a CMakeCache.txt but no build system.
    return ((build_dir / "build.ninja").is_file() or (build_dir / "Makefile").is_file()
            or any(build_dir.glob("*.sln")) or any(build_dir.glob("*.slnx")))


def _configurations(preset: str, build_dir: Path) -> list[str]:
    value = (_cache_var(build_dir, "CMAKE_CONFIGURATION_TYPES")
             or _preset_cache_var(preset, "CMAKE_CONFIGURATION_TYPES"))
    return value.split(";") if value else DEFAULT_CONFIGS


def _canonical_config(requested: str, configs: list[str]) -> str:
    for config in configs:
        if config.lower() == requested.lower():
            return config
    fatal(f"Unknown configuration '{requested}'. Available: {', '.join(configs)}")


def _install_bin_dir(build_dir: Path, config: str) -> Path | None:
    prefix = _cache_var(build_dir, "CMAKE_INSTALL_PREFIX")
    if not prefix:
        return None
    # Mirrors CX_INSTALL_ROOT in the root CMakeLists.txt:
    # Debug -> <prefix>/debug/bin, Release -> <prefix>/release/bin, Shipping -> <prefix>/bin
    root = Path(prefix) if config == "Shipping" else Path(prefix) / config.lower()
    return root / "bin"


def _installed_configs(build_dir: Path, configs: list[str]) -> list[str]:
    installed = []
    for config in configs:
        bin_dir = _install_bin_dir(build_dir, config)
        if bin_dir and (bin_dir / f"{EDITOR}{EXE_SUFFIX}").is_file():
            installed.append(config)
    return installed


def _auto_preset(available: list[str]) -> tuple[str, str]:
    """Preset to use when none is given: ``(name, reason)``."""
    configured = [p for p in available if _is_configured(_build_dir(p))]
    if configured:
        latest = max(configured, key=lambda p: (_build_dir(p) / "CMakeCache.txt").stat().st_mtime)
        return latest, "most recently configured build"
    default = DEFAULT_PRESET.get(HOST)
    return (default if default in available else available[0]), "host default"


def _select_preset(requested: str | None) -> str:
    available = _host_presets()
    if not available:
        fatal(f"No configure presets are available on this host ({HOST}).")
    if requested:
        if requested not in available:
            fatal(f"'{requested}' is not a configure preset on this host. "
                      f"Available: {', '.join(available)}")
        return requested
    preset, reason = _auto_preset(available)
    info(f"No preset given, using {preset} ({reason}).")
    return preset


# ---------------------------------------------------------------------------
# MSVC environment (Ninja + cl.exe presets outside a developer prompt)
# ---------------------------------------------------------------------------

def _needs_msvc_env(preset: str) -> bool:
    """Ninja with cl.exe needs the vcvars environment for every configure and build."""
    if HOST != "Windows" or shutil.which("cl"):
        return False
    if str(_preset_field(preset, "generator") or "").startswith("Visual Studio"):
        return False  # MSBuild sets the compiler environment up itself.
    compiler = _preset_cache_var(preset, "CMAKE_CXX_COMPILER") or ""
    return Path(compiler).stem.lower() == "cl"


def _msvc_env() -> dict[str, str]:
    """Environment of the newest Visual Studio with the C++ tools, as vcvarsall.bat x64 sets it."""
    program_files = os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)")
    vswhere = Path(program_files) / "Microsoft Visual Studio" / "Installer" / "vswhere.exe"
    if not vswhere.is_file():
        fatal("vswhere.exe not found; is Visual Studio installed?")
    res = subprocess.run(
        [str(vswhere), "-latest", "-products", "*",
         "-requires", "Microsoft.VisualStudio.Component.VC.Tools.x86.x64",
         "-property", "installationPath"],
        capture_output=True, text=True,
    )
    installs = res.stdout.strip().splitlines()
    if res.returncode != 0 or not installs:
        fatal("No Visual Studio installation with the C++ tools was found.")

    vcvars = Path(installs[0]) / "VC" / "Auxiliary" / "Build" / "vcvarsall.bat"
    info(f"cl.exe is not on PATH, loading the MSVC environment from {vcvars}")
    # A string command is passed to CreateProcess unchanged, which cmd's quoting needs.
    res = subprocess.run(f'cmd.exe /d /s /c ""{vcvars}" x64 >nul && set"',
                         capture_output=True, text=True, errors="replace")
    env: dict[str, str] = {}
    for line in res.stdout.splitlines():
        key, sep, value = line.partition("=")
        if sep and key:
            env[key] = value
    if res.returncode != 0 or not any(k.upper() == "INCLUDE" for k in env):
        fatal(f"vcvarsall.bat failed:\n{res.stdout.strip()}\n{res.stderr.strip()}")
    return env


# ---------------------------------------------------------------------------
# Steps
# ---------------------------------------------------------------------------

class _Runner:
    """Runs one build step, logs its duration and stops the script if it fails."""

    def __init__(self, preset: str, quiet: bool = False) -> None:
        self.quiet = quiet
        self.env = _msvc_env() if _needs_msvc_env(preset) else None

    def __call__(self, title: str, cmd: list[str]) -> None:
        shown = subprocess.list2cmdline(cmd) if HOST == "Windows" else " ".join(cmd)
        info(f"{title} started: {shown}")
        start = time.perf_counter()
        try:
            if self.quiet:
                res = subprocess.run(cmd, cwd=ROOT, env=self.env, stdout=subprocess.PIPE,
                                     stderr=subprocess.STDOUT, text=True, errors="replace")
            else:
                res = subprocess.run(cmd, cwd=ROOT, env=self.env)
        except FileNotFoundError:
            fatal(f"{cmd[0]} was not found on PATH.")
        if res.returncode != 0:
            if self.quiet and res.stdout:
                print(res.stdout)
            fatal(f"{title} failed (exit code {res.returncode}).")
        info(f"{title} finished. Took: {time.perf_counter() - start:.1f}s")


def _configure(run: _Runner, preset: str, fresh: bool) -> None:
    cmd = ["cmake", "--preset", preset]
    if fresh:
        cmd.append("--fresh")
    run("Configure (CMake + Conan)", cmd)


def _write_launch_json(preset: str, build_dir: Path, configs: list[str]) -> None:
    """Point .vscode/launch.json at this preset's installed editors, keeping other entries."""
    path = ROOT / ".vscode" / "launch.json"
    data = {"version": "0.2.0", "configurations": []}
    if path.is_file():
        try:
            with open(path, encoding="utf-8") as fh:
                data = json.load(fh)
        except (OSError, ValueError):
            warn(f"{path} isn't plain JSON (comments?), leaving it untouched.")
            return

    tag = f"{LAUNCH_PREFIX} [{preset} "
    kept = [c for c in data.get("configurations", []) if not c.get("name", "").startswith(tag)]
    ours = []
    for config in _installed_configs(build_dir, configs):
        bin_dir = _install_bin_dir(build_dir, config)
        ours.append({
            "name": f"{tag}{config}]",
            "type": "lldb",
            "request": "launch",
            "program": (bin_dir / f"{EDITOR}{EXE_SUFFIX}").as_posix(),
            "cwd": bin_dir.as_posix(),
            "args": [],
        })
    data["configurations"] = ours + kept

    path.parent.mkdir(exist_ok=True)
    with open(path, "w", encoding="utf-8") as fh:
        json.dump(data, fh, indent=4)
        fh.write("\n")
    info(f"Updated {path.relative_to(ROOT).as_posix()} ({len(ours)} {preset} entr{'y' if len(ours) == 1 else 'ies'}).")


def _launch(build_dir: Path, config: str, program: str, program_args: list[str], vglrun: bool) -> int:
    bin_dir = _install_bin_dir(build_dir, config)
    if bin_dir is None:
        fatal(f"{build_dir} is not configured.")
    exe = bin_dir / (program if program.endswith(EXE_SUFFIX) else program + EXE_SUFFIX)
    if not exe.is_file():
        installed = sorted(
            p.stem for p in (bin_dir.iterdir() if bin_dir.is_dir() else [])
            if p.is_file() and (p.suffix.lower() == ".exe" if HOST == "Windows" else os.access(p, os.X_OK)))
        fatal(f"{exe} not found. Installed programs: {', '.join(installed) or 'none'}")

    cmd = [str(exe), *program_args]
    if vglrun:
        if not shutil.which("vglrun"):
            fatal("vglrun was not found on PATH.")
        cmd.insert(0, "vglrun")
    info(f"Launching: {' '.join(cmd)}")
    return subprocess.run(cmd, cwd=bin_dir).returncode


# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------

def list_presets() -> int:
    available = _host_presets()
    if not available:
        fatal(f"No configure presets are available on this host ({HOST}).")
    default, _ = _auto_preset(available)
    width = max(len(p) for p in available)
    print(f"Configure presets ({HOST}), * = default:")
    for preset in available:
        build_dir = _build_dir(preset)
        if _is_configured(build_dir):
            installed = _installed_configs(build_dir, _configurations(preset, build_dir))
            state = "configured" + (f", installed: {', '.join(installed)}" if installed else "")
        else:
            state = "not configured"
        print(f"{'*' if preset == default else ' '} {preset:<{width}}  {state}")
    return 0


def build(args: argparse.Namespace) -> int:
    preset = _select_preset(args.preset)
    build_dir = _build_dir(preset)
    run = _Runner(preset, args.no_out)

    if args.fresh or args.configure or not _is_configured(build_dir):
        _configure(run, preset, args.fresh)

    configs = _configurations(preset, build_dir)
    config = _canonical_config(args.config, configs)

    if args.clean:
        run(f"Clean {preset} {config}",
            ["cmake", "--build", str(build_dir), "--config", config, "--target", "clean"])

    cmd = ["cmake", "--build", str(build_dir), "--config", config,
           "--parallel", str(1 if args.no_parallel else (args.jobs or os.cpu_count() or 1))]
    for target in args.target:
        cmd += ["--target", target]
    run(f"Build {preset} {config}", cmd)

    if args.install:
        run(f"Install {preset} {config}", ["cmake", "--install", str(build_dir), "--config", config])
        info(f"Installed to {_install_bin_dir(build_dir, config)}")
        if not args.no_launch_json:
            _write_launch_json(preset, build_dir, configs)

    if args.run:
        return _launch(build_dir, config, args.run, args.program_args, args.vglrun)
    info("Done.")
    return 0


# ---------------------------------------------------------------------------
# Argument parser
# ---------------------------------------------------------------------------

_EXAMPLES = f"""\
examples:
  build.py                                    configure if needed, build Debug
  build.py --run                              build, install and launch {EDITOR}
  build.py --preset windows-vs2026 --config release --install
  build.py --run --config shipping -- ARGS    pass ARGS to the launched program
  build.py --list                             show presets, * marks the default
"""


def _make_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="build.py",
        usage="%(prog)s [options] [-- PROGRAM_ARGS...]",
        description="Configure, build, install and run Codex from CMakePresets.json.\n"
                    "Configuring a preset also installs its Conan packages.",
        epilog=_EXAMPLES,
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )

    preset = parser.add_argument_group("preset")
    preset.add_argument("--list", action="store_true",
                        help="List this host's presets and what is configured/installed, then exit")
    preset.add_argument("--preset", metavar="NAME",
                        help="Configure preset to use. Default: the most recently configured "
                             "build, otherwise the host's default preset")
    preset.add_argument("--config", metavar="CONFIG", default="Debug",
                        help="Debug, Release or Shipping, case-insensitive (default: Debug)")
    preset.add_argument("--configure", action="store_true",
                        help="Re-run the configure step (and Conan) even if already configured, "
                             "e.g. after editing CMakePresets.json")
    preset.add_argument("--fresh", action="store_true",
                        help="Discard the CMake cache and configure from scratch")

    build_group = parser.add_argument_group("build")
    build_group.add_argument("--clean", action="store_true",
                             help="Run the 'clean' target for the configuration before building")
    build_group.add_argument("-t", "--target", action="append", default=[], metavar="TARGET",
                             help="Build only this target (repeatable). Default: all")
    build_group.add_argument("-j", "--jobs", type=int, metavar="N",
                             help="Parallel jobs (default: number of CPUs)")
    build_group.add_argument("--no-parallel", action="store_true", help="Build with a single job")

    install = parser.add_argument_group("install and run")
    install.add_argument("--install", action="store_true",
                         help="Run cmake --install for the configuration after building")
    install.add_argument("--run", nargs="?", const=EDITOR, metavar="PROGRAM",
                         help=f"Install, then launch an installed program (default: {EDITOR}). "
                              "Arguments after '--' are passed to it")
    install.add_argument("--vglrun", action="store_true",
                         help="Launch through VirtualGL's vglrun (Linux); implies --run")
    install.add_argument("--no-launch-json", action="store_true",
                         help="Don't update .vscode/launch.json after installing")

    output = parser.add_argument_group("output")
    output.add_argument("--no-out", action="store_true",
                        help="Hide CMake/Conan output unless a step fails")

    return parser


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> int:
    argv = sys.argv[1:]
    program_args: list[str] = []
    if "--" in argv:
        split = argv.index("--")
        argv, program_args = argv[:split], argv[split + 1:]

    args = _make_parser().parse_args(argv)
    if args.list:
        return list_presets()

    args.program_args = program_args
    if args.vglrun and not args.run:
        args.run = EDITOR
    if args.run:
        args.install = True
    return build(args)


if __name__ == "__main__":
    try:
        sys.exit(main())
    except KeyboardInterrupt:
        sys.exit(130)
