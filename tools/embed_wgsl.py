#!/usr/bin/env python3
import sys
from pathlib import Path


def main() -> int:
    if len(sys.argv) != 5:
        print(
            "usage: embed_wgsl.py <input.wgsl> <output.hpp> <namespace> <symbol>",
            file=sys.stderr,
        )
        return 2

    source = Path(sys.argv[1])
    output = Path(sys.argv[2])
    namespace = sys.argv[3]
    symbol = sys.argv[4]
    text = source.read_text()

    # Pick a raw-string delimiter that cant terminate inside shader source
    delimiter = "mira_wgsl"
    while f'){delimiter}"' in text:
        delimiter += "_"

    contents = (
        "#pragma once\n\n"
        f"namespace {namespace} {{\n\n"
        f'inline constexpr char {symbol}[] = R"{delimiter}(\n'
        f"{text}"
        f'){delimiter}";\n\n'
        f"}} // namespace {namespace}\n"
    )

    output.parent.mkdir(parents=True, exist_ok=True)
    # Avoid touching generated files when content is unchanged; it keeps ninja quiet.
    if output.exists() and output.read_text() == contents:
        return 0
    output.write_text(contents)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
