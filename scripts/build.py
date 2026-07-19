#!/usr/bin/env python3

import os
import sys
import json
import platform
import subprocess
import argparse

import com


_LAUNCH_TEMPLATE = {
    "version": "0.2.0",
    "configurations": [
        {
            "name": "(LLDB Debug): Codex Editor",
            "type": "lldb",
            "request": "launch",
            "program": None,
            "cwd": None,
            "args": [],
        }
    ],
}


# ---------------------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------------------

def _build_dir(preset: str) -> str:
    return os.path.join("builds", preset)


def _is_cmake_build_dir(name: str) -> bool:
    return os.path.isfile(os.path.join(_build_dir(name), "CMakeCache.txt"))


def _resolve_preset(preset: str | None, config: str | None) -> tuple[str, bool]:
    """Return ``(preset_name, needs_cmake_configure)``.

    Panics when no suitable preset can be found.
    """
    # Only directories with a CMakeCache.txt count as existing builds; builds/
    # also holds non-CMake trees (e.g. conan-debug) and half-configured leftovers.
    existing = {s for s in os.listdir("builds") if _is_cmake_build_dir(s)} \
        if os.path.isdir("builds") else set()
    available = com.get_cmake_presets()

    if not available:
        com.panic("Platform not supported.")

    if preset is not None:
        # Explicit preset — always re-configure so the user gets what they asked for.
        return preset, True

    if config is None:
        # No hints: prefer an already-generated build directory.
        common = existing.intersection(available)
        if common:
            chosen = sorted(common)[0]
            com.log(f"Auto-detected existing build: {chosen}")
            return chosen, False
        com.log(f"No preset provided, defaulting to: {available[0]}")
        return available[0], True

    # Config hint: prefer an already-generated build that contains the config string.
    matching_builds = {s for s in existing if config in s}
    if matching_builds:
        chosen = sorted(matching_builds)[0]
        com.log(f"Found existing build: {chosen}")
        return chosen, False

    matching_presets = {s for s in available if config in s}
    if matching_presets:
        chosen = sorted(matching_presets)[0]
        com.log(f"Building: {chosen}")
        return chosen, True

    com.panic(f"No preset found for config '{config}'.")


def _editor_binary() -> str:
    """Return the editor binary path relative to the install prefix."""
    name = "CodexEditor.exe" if platform.system() == "Windows" else "CodexEditor"
    return os.path.join("bin", name)


def _gen_launch_file(preset: str) -> None:
    inspector = com.CMakeInspector(_build_dir(preset))
    install_dir = inspector.get_var("CMAKE_INSTALL_PREFIX")
    data = _LAUNCH_TEMPLATE.copy()
    data["configurations"][0]["program"] = os.path.join(install_dir, _editor_binary())
    data["configurations"][0]["cwd"] = install_dir
    os.makedirs(".vscode", exist_ok=True)
    with open(os.path.join(".vscode", "launch.json"), "w") as fh:
        json.dump(data, fh, indent=4)
    com.log("launch.json updated.")


# ---------------------------------------------------------------------------
# Sub-commands
# ---------------------------------------------------------------------------

def cmd_list(_args: argparse.Namespace) -> None:
    presets = com.get_cmake_presets()
    if not presets:
        com.panic("No presets found (platform may be unsupported).")
    com.log("Available presets:")
    for p in presets:
        print(f"  {p}")


def cmd_build(args: argparse.Namespace) -> None:
    stdout = subprocess.DEVNULL if args.no_out else None
    parallel_flag = f"--parallel {os.cpu_count()}" if not args.no_parallel else ""

    preset, needs_configure = _resolve_preset(args.preset, args.config)
    build_path = _build_dir(preset)

    def _cmake_conf() -> None:
        com.Chrono.begin()
        com.log("CMake configuration started.")
        res = com.run(
            f"cmake --preset={preset} -DCMAKE_POLICY_VERSION_MINIMUM=3.5",
            stdout=stdout,
            stderr=stdout,
        )
        if res.returncode != 0:
            if preset not in com.get_cmake_presets():
                com.panic(f"'{preset}' is not a valid preset.")
            com.panic("CMake configuration failed.")
        elapsed = com.Chrono.end()
        com.log(f"CMake configuration finished. Took: {elapsed:.2f}ms")

    def _conan_install() -> None:
        com.Chrono.begin()
        com.log("Conan install started.")
        res = com.run(
            f"conan install . --output-folder=builds/conan-{args.lib_config.lower()} --build=missing -s build_type={args.lib_config.lower().capitalize()}",
            stdout=stdout,
            stderr=stdout,
        )
        if res.returncode != 0:
            com.panic("Conan installation failed.")
        elapsed = com.Chrono.end()
        com.log(f"Conan installation finished. Took: {elapsed:.2f}ms")

    if args.conan_sync:
        _conan_install()

    if needs_configure:
        _cmake_conf()


    com.Chrono.begin()
    res = com.run(
        f"cmake --build {build_path} {parallel_flag}".strip(),
        stdout=stdout,
    )
    if res.returncode != 0:
        com.panic("Build failed.")
    elapsed = com.Chrono.end()
    com.log(f"CMake build finished. Took: {elapsed:.2f}ms")

    if args.install:
        com.Chrono.begin()
        com.log("CMake installation started.")
        res = com.run(f"cmake --install {build_path}", stdout=stdout)
        if res.returncode != 0:
            com.panic("CMake install failed.")
        elapsed = com.Chrono.end()
        _gen_launch_file(preset)
        com.log(f"CMake installation finished. Took: {elapsed:.2f}ms")

    if args.run:
        inspector = com.CMakeInspector(build_path)
        install_dir = inspector.get_var("CMAKE_INSTALL_PREFIX")
        editor = os.path.join(install_dir, _editor_binary())
        if not os.path.isfile(editor):
            com.panic(f"Editor binary not found: {editor}\nRun with --install first.")
        cmd = ["vglrun", editor] if args.vglrun else [editor]
        com.log(f"Launching: {' '.join(cmd)}")
        subprocess.run(cmd, cwd=install_dir)


def cmd_clear(args: argparse.Namespace) -> None:
    stdout = subprocess.DEVNULL if args.no_out else None

    preset = args.preset
    if preset is None:
        presets = com.get_cmake_presets()
        if not presets:
            com.panic("Platform not supported.")
        preset = presets[0]
        com.log(f"No preset specified, defaulting to: {preset}")

    com.Chrono.begin()
    com.run(f"cmake --build {_build_dir(preset)} --target clean", stdout=stdout)
    elapsed = com.Chrono.end()
    com.log(f"CMake clean finished. Took: {elapsed:.2f}ms")


# ---------------------------------------------------------------------------
# Argument parser
# ---------------------------------------------------------------------------

def _make_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="build.py",
        description="Codex Engine build helper",
    )
    sub = parser.add_subparsers(dest="command", metavar="<command>")

    # -- list ----------------------------------------------------------------
    sub.add_parser("list", help="List available CMake configure presets")

    # -- build ---------------------------------------------------------------
    build = sub.add_parser("build", help="Configure and build the project (default)")
    build.add_argument(
        "--preset", metavar="NAME",
        help="CMake configure preset (e.g. linux-any-debug)",
    )
    build.add_argument(
        "--config", metavar="TYPE",
        help="Build configuration substring to match (e.g. debug, release)",
        default="debug",
    )
    build.add_argument(
        "--lib-config", metavar="TYPE",
        dest="lib_config",
        help="Library build configuration for Conan (e.g. debug, release). Defaults to --config.",
        default=None,
    )
    build.add_argument(
        "--no-parallel", action="store_true",
        help="Disable parallel compilation",
    )
    build.add_argument(
        "--no-out", action="store_true",
        help="Suppress CMake stdout/stderr",
    )
    build.add_argument(
        "--install", action="store_true",
        help="Run cmake --install after a successful build",
    )
    build.add_argument(
        "--run", action="store_true",
        help="Launch the editor after installation (implies --install)",
    )
    build.add_argument(
        "--vglrun", action="store_true",
        help="Prefix the editor launch with vglrun (VirtualGL; implies --run)",
    )
    build.add_argument("--conan-sync", action="store_true",
        dest="conan_sync",
        help="Synchronize Conan packages"
    )

    # -- clear ---------------------------------------------------------------
    clear = sub.add_parser("clear", help="Run the CMake 'clean' target")
    clear.add_argument(
        "--preset", metavar="NAME",
        help="Preset whose build directory to clean",
    )
    clear.add_argument(
        "--no-out", action="store_true",
        help="Suppress CMake stdout/stderr",
    )

    return parser


# ---------------------------------------------------------------------------
# Entry point
# ---------------------------------------------------------------------------

def main() -> None:
    parser = _make_parser()

    # When invoked with no subcommand (or only build-related flags), default
    # to the 'build' subcommand so existing one-liner usage still works.
    if not sys.argv[1:] or sys.argv[1].startswith("-"):
        sys.argv.insert(1, "build")

    args = parser.parse_args()

    if args.command is None:
        parser.print_help()
        sys.exit(0)

    # --vglrun implies --run; --run implies --install
    if args.command == "build" and args.vglrun:
        args.run = True
    if args.command == "build" and args.run:
        args.install = True
    if args.command == "build" and args.lib_config is None:
        args.lib_config = args.config

    dispatch = {
        "list": cmd_list,
        "build": cmd_build,
        "clear": cmd_clear,
    }
    dispatch[args.command](args)
    com.log("Done.")


if __name__ == "__main__":
    main()
