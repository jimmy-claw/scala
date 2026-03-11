BUILD_DIR        ?= build
BUILD_STANDALONE ?= build-standalone
BUILD_MODULE_DIR ?= build-module
CMAKE_FLAGS      ?= -DCMAKE_BUILD_TYPE=Debug
TOOLS_DIR        ?= ./tools
MODULES_DIR      ?= ./modules

# ── Nix store auto-detection ─────────────────────────────────────────────────
# Split packages: headers, lib, and bin are separate nix outputs
LOGOS_HEADERS_NIX ?= $(shell ls -d /nix/store/*logos-liblogos-headers-* 2>/dev/null | grep -v '\.drv$$' | head -1)
LOGOS_LIB_NIX     ?= $(shell ls -d /nix/store/*logos-liblogos-lib-* 2>/dev/null | grep -v '\.drv$$' | head -1)
LOGOS_BIN_NIX     ?= $(shell ls -d /nix/store/*logos-liblogos-bin-* 2>/dev/null | grep -v '\.drv$$' | head -1)
LOGOS_SDK_HEADERS_NIX ?= $(shell ls -d /nix/store/*logos-cpp-sdk-headers-* 2>/dev/null | grep -v '\.drv$$' | head -1)
LOGOS_SDK_LIB_NIX     ?= $(shell ls -d /nix/store/*logos-cpp-sdk-lib-* 2>/dev/null | grep -v '\.drv$$' | head -1)

# Fallback: unsplit package (from nix build)
LOGOS_LIBLOGOS_NIX ?= $(shell ls -d $(TOOLS_DIR)/logoscore 2>/dev/null)

# logoscore binary: prefer split bin package, fall back to tools dir
LOGOSCORE ?= $(if $(LOGOS_BIN_NIX),$(LOGOS_BIN_NIX)/bin/logoscore,$(TOOLS_DIR)/logoscore/bin/logoscore)

# Nix Qt paths for building against nix Qt 6.9
NIX_QTBASE     ?= $(shell ls -d /nix/store/*-qtbase-6.9.* 2>/dev/null | grep -v '\.drv$$' | grep -v dev | grep -v plugins | head -1)
NIX_QTDECL     ?= $(shell ls -d /nix/store/*-qtdeclarative-6.9.* 2>/dev/null | grep -v '\.drv$$' | grep -v dev | head -1)
NIX_QTREMOBJ   ?= $(shell ls -d /nix/store/*-qtremoteobjects-6.9.* 2>/dev/null | grep -v '\.drv$$' | grep -v dev | head -1)
NIX_QT_PREFIX  ?= $(NIX_QTBASE);$(NIX_QTDECL);$(NIX_QTREMOBJ)

.PHONY: all build test test-cli clean standalone screenshot \
        setup setup-logoscore setup-kv-module setup-nix-merged \
        build-module run-module run-core run dev install-cli

# ── Build ────────────────────────────────────────────────────────────────────

all: build

build:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. $(CMAKE_FLAGS) && make -j$$(nproc)

standalone:
	mkdir -p $(BUILD_STANDALONE)
	cd $(BUILD_STANDALONE) && cmake .. $(CMAKE_FLAGS) \
		-DBUILD_STANDALONE=ON \
		$(if $(LOGOS_HEADERS_NIX),-DLOGOS_CPP_SDK_ROOT=/tmp/logos-cpp-sdk-merged -DLOGOS_LIBLOGOS_ROOT=/tmp/logos-liblogos-merged,) \
		$(if $(NIX_QTBASE),-DCMAKE_PREFIX_PATH="$(NIX_QT_PREFIX)" -DQT_ADDITIONAL_PACKAGES_PREFIX_PATH="$(NIX_QTDECL)$$(echo ';')$(NIX_QTREMOBJ)",) \
		&& cmake --build . -j$$(nproc) --target scala_standalone

test: build
	cd $(BUILD_DIR) && ctest --output-on-failure -V

test-cli:
	@echo "Running CLI integration tests (requires make run-core in another terminal)..."
	bash tests/cli/test_cli.sh ./cli/scala-cli.sh

clean:
	rm -rf $(BUILD_DIR) $(BUILD_STANDALONE) $(BUILD_MODULE_DIR)

# ── Logoscore module plugin ───────────────────────────────────────────────────

## Build scala as logoscore plugin (.so for modules/scala_module/)
build-module: setup-nix-merged
	cmake -B $(BUILD_MODULE_DIR) $(CMAKE_FLAGS) \
		-DBUILD_MODULE=ON \
		-DLOGOS_CPP_SDK_ROOT=/tmp/logos-cpp-sdk-merged \
		-DLOGOS_LIBLOGOS_ROOT=/tmp/logos-liblogos-merged \
		$(if $(NIX_QTBASE),-DCMAKE_PREFIX_PATH="$(NIX_QT_PREFIX)" -DQT_ADDITIONAL_PACKAGES_PREFIX_PATH="$(NIX_QTDECL)$$(echo ';')$(NIX_QTREMOBJ)",)
	cmake --build $(BUILD_MODULE_DIR) -j$$(nproc) --target scala_module_plugin
	mkdir -p $(MODULES_DIR)/scala_module
	cp $(BUILD_MODULE_DIR)/scala_module_plugin.so $(MODULES_DIR)/scala_module/
	@echo "scala_module ready at: $(MODULES_DIR)/scala_module/"

## Run as proper logoscore module (kv_module + scala_module)
run-module:
	$(LOGOSCORE) --modules-dir $(MODULES_DIR) --load-modules kv_module,scala_module

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

## Create merged symlink dirs for split nix packages
setup-nix-merged:
	@if [ -z "$(LOGOS_HEADERS_NIX)" ]; then echo "ERROR: logos-liblogos-headers not found in nix store."; exit 1; fi
	@if [ -z "$(LOGOS_SDK_HEADERS_NIX)" ]; then echo "ERROR: logos-cpp-sdk-headers not found in nix store."; exit 1; fi
	@echo "Creating merged SDK dirs from split nix packages..."
	rm -rf /tmp/logos-cpp-sdk-merged /tmp/logos-liblogos-merged
	mkdir -p /tmp/logos-cpp-sdk-merged/include /tmp/logos-cpp-sdk-merged/lib
	ln -sf $(LOGOS_SDK_HEADERS_NIX)/include/* /tmp/logos-cpp-sdk-merged/include/
	ln -sf $(LOGOS_SDK_LIB_NIX)/lib/* /tmp/logos-cpp-sdk-merged/lib/
	mkdir -p /tmp/logos-liblogos-merged/include /tmp/logos-liblogos-merged/lib
	ln -sf $(LOGOS_HEADERS_NIX)/include/* /tmp/logos-liblogos-merged/include/
	ln -sf $(LOGOS_LIB_NIX)/lib/* /tmp/logos-liblogos-merged/lib/
	@echo "Merged dirs ready at /tmp/logos-{cpp-sdk,liblogos}-merged/"

## Build and install kv_module (requires split nix packages or nix build output)
setup-kv-module: setup-nix-merged
	@echo "Building kv_module with Logos Core headers..."
	rm -rf /tmp/logos-kv-module
	git clone --depth 1 https://github.com/jimmy-claw/logos-kv-module /tmp/logos-kv-module
	cd /tmp/logos-kv-module && cmake -B build-logos \
		-DCMAKE_BUILD_TYPE=Release \
		-DLOGOS_CPP_SDK_ROOT=/tmp/logos-cpp-sdk-merged \
		-DLOGOS_LIBLOGOS_ROOT=/tmp/logos-liblogos-merged \
		$(if $(NIX_QTBASE),-DCMAKE_PREFIX_PATH="$(NIX_QT_PREFIX)" -DQT_ADDITIONAL_PACKAGES_PREFIX_PATH="$(NIX_QTDECL)$$(echo ';')$(NIX_QTREMOBJ)",) \
		&& cmake --build build-logos -j$$(nproc)
	mkdir -p $(MODULES_DIR)/kv_module
	cp $$(find /tmp/logos-kv-module/build-logos -name '*.so' | head -1) $(MODULES_DIR)/kv_module/kv_module_plugin.so
	echo '{"name":"kv_module","version":"0.1.0","main":{"linux-x86_64":"kv_module_plugin.so","linux-aarch64":"kv_module_plugin.so","darwin-arm64":"kv_module_plugin.so","darwin-x86_64":"kv_module_plugin.so"}}' > $(MODULES_DIR)/kv_module/manifest.json
	@echo "kv_module ready at: $(MODULES_DIR)/kv_module/"

## Full setup: logoscore + kv_module
setup: setup-logoscore setup-kv-module
	@echo ""
	@echo "Setup complete! Run 'make dev' to start."

# ── CLI ──────────────────────────────────────────────────────────────────────

install-cli:
	install -m 755 cli/scala-cli.sh ~/.local/bin/scala-cli
	@echo "scala-cli installed to ~/.local/bin/scala-cli"

# ── Run ──────────────────────────────────────────────────────────────────────

## Start Logos Core with kv_module (run in separate terminal)
run-core:
	$(LOGOSCORE) --modules-dir $(MODULES_DIR) --load-modules kv_module

## Run Scala standalone (connects to Logos Core if running)
run: standalone
	QT_QPA_PLATFORM=offscreen \
	QT_PLUGIN_PATH=$(NIX_QTBASE)/lib/qt-6/plugins \
	QML_IMPORT_PATH=$(NIX_QTDECL)/lib/qt-6/qml \
	LOGOS_CORE_AVAILABLE=1 \
	./$(BUILD_STANDALONE)/scala_standalone

## Full dev stack: build everything and run
## Run 'make run-core' in a separate terminal first
dev: standalone
	@echo "Starting Scala..."
	@echo "TIP: Run 'make run-core' in another terminal for persistent storage."
	QT_PLUGIN_PATH=$(NIX_QTBASE)/lib/qt-6/plugins \
	QML_IMPORT_PATH=$(NIX_QTDECL)/lib/qt-6/qml \
	LOGOS_CORE_AVAILABLE=1 \
	./$(BUILD_STANDALONE)/scala_standalone
