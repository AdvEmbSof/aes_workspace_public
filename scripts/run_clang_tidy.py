#!/usr/bin/env python3

from pathlib import Path
import subprocess
import sys
import re

def run(cmd):
    print("+", " ".join(cmd))
    subprocess.run(cmd, check=True)


def split_spec(spec: str):
    return [s for s in re.split(r"[+,]", spec or "") if s]


def build_database(app: str, spec: str):
    cmd = [
        sys.executable,
        "scripts/build.py",
        app,
        spec,
        "yes",
        "--native_sim",
    ]
    run(cmd)


def filter_database():
    Path("build_clang").mkdir(exist_ok=True)

    run([
        sys.executable,
        "scripts/filter_compile_commands.py",
        "build/compile_commands.json",
        "build_clang/compile_commands.json",
    ])


def run_clang_tidy_patterns(workdir: str, app: str):
    patterns = [
        rf"{workdir}/{app}/src/.*\.cpp$",
        rf"{workdir}/deps/zpp_lib/.*\.cpp$",
    ]

    for pattern in patterns:
        run([
            "run-clang-tidy",
            "-p",
            "build_clang",
            pattern,
            "--warning-as-error *",
            "-quiet",
        ])


def run_clang_tidy_files(files):
    for f in files:
        if not f.endswith((".cpp", ".cc", ".cxx")):
            continue

        run([
            "clang-tidy",
            "-p",
            "build_clang",
            f,
            "--warnings-as-errors=*",
        ])


def main():
    args = sys.argv[1:]

    # -----------------------------
    # Mode detection
    # -----------------------------
    if args and args[0] == "--files":
        files = args[1:]
        print("Running clang-tidy on files:", files)
        run_clang_tidy_files(files)
        return

    # -----------------------------
    # Full mode
    # -----------------------------
    app = args[0] if len(args) > 0 else "bike_computer"
    spec = args[1] if len(args) > 1 else ""
    workdir = args[2] if len(args) > 2 else "."

    print(f"App: {app}")
    print(f"Spec: {spec}")

    build_database(app, spec)
    filter_database()
    run_clang_tidy_patterns(workdir, app)


if __name__ == "__main__":
    main()