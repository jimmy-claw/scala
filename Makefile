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

## Build and install kv_module
setup-kv-module:
	@echo "Building kv_module..."
	nix --extra-experimental-features 'nix-command flakes' build github:jimmy-claw/logos-kv-module#kv_module-lib -o /tmp/logos-kv-module-result
	mkdir -p $(MODULES_DIR)/kv_module
	find /tmp/logos-kv-module-result -name '*.so' | head -1 | xargs -I{} cp {} $(MODULES_DIR)/kv_module/kv_module_plugin.so
	echo '{"name":"kv_module","version":"0.1.0","main":{"linux-x86_64":"kv_module_plugin.so","linux-aarch64":"kv_module_plugin.so","darwin-arm64":"kv_module_plugin.so","darwin-x86_64":"kv_module_plugin.so"}}' > $(MODULES_DIR)/kv_module/manifest.json
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
