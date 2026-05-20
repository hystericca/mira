# Mira

C++23 Dawn/Emdawnwebgpu WebGPU canvas.

`DAWN_REVISION` pins the Dawn checkout this tree was tested with.
Dawn must be synced with `scripts/standalone-with-wasm.gclient`.

`make check` runs typecheck, native build, tests, bench, and probe.
`make web` builds `wasm/mira_web.html` through Emdawn and serves it.
`make release` builds release native and wasm outputs.
