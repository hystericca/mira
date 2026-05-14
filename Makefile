DAWN_ROOT ?= $(HOME)/Developer/dawn
DEPOT_TOOLS ?= $(HOME)/Developer/depot_tools
OUT_DIR ?= $(DAWN_ROOT)/out/mira-debug
RELEASE_OUT_DIR ?= $(DAWN_ROOT)/out/mira-release
WEB_ROOT := $(OUT_DIR)/wasm

AUTONINJA := PATH="$(DEPOT_TOOLS):$(DAWN_ROOT)/buildtools/mac:$$PATH" "$(DEPOT_TOOLS)/autoninja"

.PHONY: all check web release clean help

all: check

check:
	@ant exec tsgo --noEmit -p tsconfig.json
	@./tools/gn_gen.sh "$(OUT_DIR)" debug
	@$(AUTONINJA) -C "$(OUT_DIR)" mira:all
	@"$(OUT_DIR)/mira_tests"
	@"$(OUT_DIR)/mira_draw_bench"
	@"$(OUT_DIR)/mira"

web:
	@ant exec tsgo --noEmit -p tsconfig.json
	@./tools/gn_gen.sh "$(OUT_DIR)" debug
	@$(AUTONINJA) -C "$(OUT_DIR)" "wasm/mira_web.html"
	@MIRA_WEB_ROOT="$(WEB_ROOT)" ant tools/serve.ts

release:
	@ant exec tsgo --noEmit -p tsconfig.json
	@./tools/gn_gen.sh "$(RELEASE_OUT_DIR)" release
	@$(AUTONINJA) -C "$(RELEASE_OUT_DIR)" mira:all
	@$(AUTONINJA) -C "$(RELEASE_OUT_DIR)" "wasm/mira_web.html"
	@"$(RELEASE_OUT_DIR)/mira_tests"
	@"$(RELEASE_OUT_DIR)/mira_draw_bench"
	@"$(RELEASE_OUT_DIR)/mira"

clean:
	@rm -rf "$(OUT_DIR)" "$(RELEASE_OUT_DIR)"
	@rm -f compile_commands.json

help:
	@echo "make          run typecheck, native build, tests, bench, and native probe"
	@echo "make check    same as make"
	@echo "make web      build browser WebGPU output and start the dev server"
	@echo "make release  release build, web build, tests, bench, and native probe"
	@echo "make clean    remove Mira GN build outputs"
