#!/usr/bin/env python3
import json
import shlex
import sys
from pathlib import Path


def keep_entry(entry):
    file_path = entry.get("file", "")
    return file_path.startswith("../../mira/") or "/mira/" in file_path


def keep_arg(arg):
    if arg.startswith("-fsanitize="):
        return False
    if arg.startswith("-fsanitize-trap="):
        return False
    if arg.startswith("-fsanitize-ignore-for-ubsan-feature="):
        return False
    if arg.startswith("-fcrash-diagnostics-dir="):
        return False
    return arg not in {
        "-fdiagnostics-show-inlining-chain",
        "-fno-lifetime-dse",
        "-gseparate-dwarf",
        "-gsimple-template-names",
        "-Wunsafe-buffer-usage",
        "-Wno-error=unsafe-buffer-usage",
        "-Wno-unsafe-buffer-usage-in-static-sized-array",
    }


def emscripten_args(args):
    if not args:
        return []
    compiler = Path(args[0])
    if compiler.name not in {"em++", "emcc"}:
        return []
    if compiler.parent.name != "emscripten":
        return []

    upstream = compiler.parent.parent
    sysroot = upstream / "emscripten" / "cache" / "sysroot" / "include"
    return [
        "--target=wasm32-unknown-emscripten",
        "-D__EMSCRIPTEN__=1",
        "-isystem",
        str(sysroot / "fakesdl"),
        "-isystem",
        str(sysroot / "compat"),
        "-isystem",
        str(sysroot / "c++" / "v1"),
        "-isystem",
        str(sysroot),
    ]


def scrub_command(command):
    args = shlex.split(command)
    scrubbed = []
    extra = emscripten_args(args)
    index = 0
    while index < len(args):
        arg = args[index]
        if arg in {"-Xclang", "-mllvm"}:
            index += 2
            continue
        if keep_arg(arg):
            scrubbed.append(arg)
        index += 1
    if extra:
        scrubbed[1:1] = extra
    return shlex.join(scrubbed)


def main():
    if len(sys.argv) != 3:
        print(
            "usage: write_clangd_database.py <gn compile_commands> <out>",
            file=sys.stderr,
        )
        return 2

    source = Path(sys.argv[1])
    output = Path(sys.argv[2])
    entries = json.loads(source.read_text())
    filtered = []
    for entry in entries:
        if not keep_entry(entry):
            continue
        next_entry = dict(entry)
        if "command" in next_entry:
            next_entry["command"] = scrub_command(next_entry["command"])
        filtered.append(next_entry)

    if output.is_symlink():
        output.unlink()
    output.write_text(json.dumps(filtered, indent=2) + "\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
