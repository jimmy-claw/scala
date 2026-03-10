BUILD_DIR        ?= build
BUILD_STANDALONE ?= build-standalone
CMAKE_FLAGS      ?= -DCMAKE_BUILD_TYPE=Debug
TOOLS_DIR        ?= ./tools
MODULES_DIR      ?= ./modules
LOGOSCORE        ?= $(TOOLS_DIR)/logoscore/bin/logoscore

.PHONY: all build test clean standalone screenshot \
        setup setup-logoscore setup-kv-module \
        run-core run dev

# ── Build ────────────────────────────────────────────────────────────────────

all: build

build:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. $(CMAKE_FLAGS) && make -j$$(nproc)

standalone:
	mkdir -p $(BUILD_STANDALONE)
	cd $(BUILD_STANDALONE) && cmake .. $(CMAKE_FLAGS) -DBUILD_STANDALONE=ON && cmake --build . -j$$(nproc) --target scala_standalone

test: build
	cd $(BUILD_DIR) && ctest --output-on-failure -V

clean:
	rm -rf $(BUILD_DIR) $(BUILD_STANDALONE)

# ── Screenshot ───────────────────────────────────────────────────────────────

screenshot: standalone
	bash scripts/screenshot.sh $(BUILD_STANDALONE)

# ── Logos Core setup ─────────────────────────────────────────────────────────

## Download and build logoscore via Nix (~10-30 min first time, cached after)
setup-logoscore:
	@echo "Building logoscore via Nix (this may take a while on first run)..."
	mkdir -p $(TOOLS_DIR)
	nix --extra-experimental-features 'nix-command flakes' build github:logos-co/logos-liblogos -o $(TOOLS_DIR)/logoscore
	@echo "logoscore ready at: $(LOGOSCORE)"

## Build and install kv_module (requires logos-liblogos headers from nix)
## Set LOGOS_LIBLOGOS_NIX to the nix store path of logos-liblogos-build
## e.g. make setup-kv-module LOGOS_LIBLOGOS_NIX=/nix/store/<hash>-logos-liblogos-build-0.1.0
LOGOS_LIBLOGOS_NIX ?= $(shell ls -d /nix/store/*logos-liblogos-build* 2>/dev/null | head -1)

setup-kv-module:
	@echo "Building kv_module with Logos Core headers..."
	@if [ -z "$(LOGOS_LIBLOGOS_NIX)" ]; then echo "ERROR: logos-liblogos not found in nix store. Run 'make setup-logoscore' first."; exit 1; fi
	@echo "Using logos-liblogos at: $(LOGOS_LIBLOGOS_NIX)"
	mkdir -p /tmp/logos-kv-module
	cd /tmp/logos-kv-module && git clone --depth 1 https://github.com/jimmy-claw/logos-kv-module . 2>/dev/null || git pull
	cd /tmp/logos-kv-module && cmake -B build-logos 		-DCMAKE_BUILD_TYPE=Release 		-DLOGOS_CORE_AVAILABLE=ON 		-DCMAKE_CXX_FLAGS="-I$(LOGOS_LIBLOGOS_NIX)/include" 		&& cmake --build build-logos -j$$(nproc)
	mkdir -p $(MODULES_DIR)/kv_module
	cp $$(find /tmp/logos-kv-module/build-logos -name '*.so' | head -1) $(MODULES_DIR)/kv_module/kv_module_plugin.so
	echo '{"name":"kv_module","version":"0.1.0","main":{"linux-x86_64":"kv_module_plugin.so","linux-aarch64":"kv_module_plugin.so","darwin-arm64":"kv_module_plugin.so","darwin-x86_64":"kv_module_plugin.so"}}' > $(MODULES_DIR)/kv_module/manifest.json
	@echo "kv_module ready at: $(MODULES_DIR)/kv_module/"
	@echo "kv_module ready at: $(MODULES_DIR)/kv_module/"

## Full setup: logoscore + kv_module
setup: setup-logoscore setup-kv-module
	@echo ""
	@echo "Setup complete! Run 'make dev' to start."

# ── Run ──────────────────────────────────────────────────────────────────────

## Start Logos Core with kv_module (run in separate terminal)
run-core:
	$(LOGOSCORE) --modules-dir $(MODULES_DIR) --load-modules kv_module

## Run Scala standalone (connects to Logos Core if running)
run: standalone
	LOGOS_CORE_AVAILABLE=1 ./$(BUILD_STANDALONE)/scala_standalone

## Full dev stack: build everything and run
## Run 'make run-core' in a separate terminal first
dev: standalone
	@echo "Starting Scala..."
	@echo "TIP: Run 'make run-core' in another terminal for persistent storage."
	LOGOS_CORE_AVAILABLE=1 ./$(BUILD_STANDALONE)/scala_standalone
