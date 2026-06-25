#!/usr/bin/env python3

from pathlib import Path
import subprocess
import sys
import re
import argparse

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
    ]

    for pattern in patterns:
        run([
            "run-clang-tidy-22",
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
            "clang-tidy-22",
            "-p",
            "build_clang",
            f,
            "--warnings-as-errors=*",
            "-quiet",
        ])


def main():
    
    parser = argparse.ArgumentParser()
    parser.add_argument("--app", required=True)
    parser.add_argument("--specs", required=True)
    parser.add_argument("--wd", required=True)
    parser.add_argument("files", nargs="*")

    args = parser.parse_args()
    
    print(f"App: {args.app}")
    print(f"Spec: {args.specs}")

    build_database(args.app, args.specs)
    filter_database()
    if args.files:
        print("Running clang-tidy on files:")
        for f in args.files:
            print(f"  {f}")
        run_clang_tidy_files(args.files)
    else:
        print("Running clang-tidy on app:" + args.app)
        run_clang_tidy_patterns(args.wd, args.app)

if __name__ == "__main__":
    main()