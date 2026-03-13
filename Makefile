BUILD_DIR        ?= build
CMAKE_FLAGS      ?= -DCMAKE_BUILD_TYPE=Debug

# Explicit SDK paths (override nix auto-detection when passed on command line)
LOGOS_CPP_SDK_ROOT  ?=
LOGOS_LIBLOGOS_ROOT ?=

# ── Nix store auto-detection ─────────────────────────────────────────────────
# Split packages: headers, lib, and bin are separate nix outputs
LOGOS_HEADERS_NIX ?= $(shell ls -d /nix/store/*logos-liblogos-headers-* 2>/dev/null | grep -v '\.drv$$' | head -1)
LOGOS_LIB_NIX     ?= $(shell ls -d /nix/store/*logos-liblogos-lib-* 2>/dev/null | grep -v '\.drv$$' | head -1)
LOGOS_SDK_HEADERS_NIX ?= $(shell ls -d /nix/store/*logos-cpp-sdk-headers-* 2>/dev/null | grep -v '\.drv$$' | head -1)
LOGOS_SDK_LIB_NIX     ?= $(shell ls -d /nix/store/*logos-cpp-sdk-lib-* 2>/dev/null | grep -v '\.drv$$' | head -1)

# Nix Qt paths for building against nix Qt 6.9
NIX_QTBASE     ?= $(shell ls -d /nix/store/*-qtbase-6.9.* 2>/dev/null | grep -v '\.drv$$' | grep -v dev | grep -v plugins | head -1)
NIX_QTDECL     ?= $(shell ls -d /nix/store/*-qtdeclarative-6.9.* 2>/dev/null | grep -v '\.drv$$' | grep -v dev | head -1)
NIX_QTREMOBJ   ?= $(shell ls -d /nix/store/*-qtremoteobjects-6.9.* 2>/dev/null | grep -v '\.drv$$' | grep -v dev | head -1)
NIX_QT_PREFIX  ?= $(NIX_QTBASE);$(NIX_QTDECL);$(NIX_QTREMOBJ)

MODULES_DIR    ?= ./modules

.PHONY: all build test clean setup-nix-merged \
        build-module build-ui-plugin install install-module \
        build-kv-module install-kv-module \
        install-delivery-module install-all

# ── Build ────────────────────────────────────────────────────────────────────

all: build

build:
	mkdir -p $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. $(CMAKE_FLAGS) && make -j$$(nproc)

test: build
	cd $(BUILD_DIR) && ctest --output-on-failure -V

clean:
	rm -rf $(BUILD_DIR) $(BUILD_MODULE) $(BUILD_UI_PLUGIN)

# ── Nix merged SDK dirs ──────────────────────────────────────────────────────

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

# ── Headless module plugin (for logoscore) ──────────────────────────────────

BUILD_MODULE ?= build-module

## Build headless logoscore plugin (no Qt Quick/GUI)
build-module: setup-nix-merged
	mkdir -p $(BUILD_MODULE)
	cd $(BUILD_MODULE) && cmake .. $(CMAKE_FLAGS) \
		-DBUILD_MODULE=ON \
		-DLOGOS_CPP_SDK_ROOT=/tmp/logos-cpp-sdk-merged \
		-DLOGOS_LIBLOGOS_ROOT=/tmp/logos-liblogos-merged \
		$(if $(NIX_QTBASE),-DCMAKE_PREFIX_PATH="$(NIX_QT_PREFIX)" -DQT_ADDITIONAL_PACKAGES_PREFIX_PATH="$(NIX_QTDECL)$$(echo ';')$(NIX_QTREMOBJ)",) \
		&& cmake --build . --target scala_module_plugin -j$$(nproc)
	mkdir -p $(MODULES_DIR)/scala_module
	cp $(BUILD_MODULE)/scala_module_plugin.so $(MODULES_DIR)/scala_module/
	cp metadata.json $(MODULES_DIR)/scala_module/manifest.json
	@echo "scala_module ready at: $(MODULES_DIR)/scala_module/"

## Install scala_module to logos-app modules dir
install-module: build-module
	mkdir -p ~/.local/share/Logos/LogosAppNix/modules/scala_module
	cp $(BUILD_MODULE)/scala_module_plugin.so ~/.local/share/Logos/LogosAppNix/modules/scala_module/
	cp metadata.json ~/.local/share/Logos/LogosAppNix/modules/scala_module/manifest.json
	@echo "scala_module installed to ~/.local/share/Logos/LogosAppNix/modules/scala_module/"

# ── UI Plugin (IComponent for logos-app) ─────────────────────────────────────

BUILD_UI_PLUGIN ?= build-ui-plugin

## Build IComponent UI plugin for logos-app
build-ui-plugin: setup-nix-merged
	mkdir -p $(BUILD_UI_PLUGIN)
	cd $(BUILD_UI_PLUGIN) && cmake .. $(CMAKE_FLAGS) \
		-DBUILD_UI_PLUGIN=ON \
		-DLOGOS_CPP_SDK_ROOT=/tmp/logos-cpp-sdk-merged \
		-DLOGOS_LIBLOGOS_ROOT=/tmp/logos-liblogos-merged \
		$(if $(NIX_QTBASE),-DCMAKE_PREFIX_PATH="$(NIX_QT_PREFIX)" -DQT_ADDITIONAL_PACKAGES_PREFIX_PATH="$(NIX_QTDECL)$$(echo ';')$(NIX_QTREMOBJ)",) \
		&& cmake --build . --target scala_ui -j$$(nproc)
	@echo "scala_ui plugin ready at: $(BUILD_UI_PLUGIN)/libscala_ui.so"

## Install scala_ui plugin to logos-app plugins dir
install: build-ui-plugin
	mkdir -p ~/.local/share/Logos/LogosAppNix/plugins/scala_ui
	cp $(BUILD_UI_PLUGIN)/libscala_ui.so ~/.local/share/Logos/LogosAppNix/plugins/scala_ui/scala_ui.so
	@echo "scala_ui installed to ~/.local/share/Logos/LogosAppNix/plugins/scala_ui/scala_ui.so"

# ── kv_module (persistence backend) ──────────────────────────────────────────

KV_MODULE_DIR ?= /tmp/logos-kv-module

## Clone and build logos-kv-module from source
build-kv-module: setup-nix-merged
	@echo "Building kv_module from jimmy-claw/logos-kv-module..."
	rm -rf $(KV_MODULE_DIR)
	git clone --depth 1 https://github.com/jimmy-claw/logos-kv-module $(KV_MODULE_DIR)
	cd $(KV_MODULE_DIR) && cmake -B build \
		-DCMAKE_BUILD_TYPE=Release \
		-DLOGOS_CPP_SDK_ROOT=/tmp/logos-cpp-sdk-merged \
		-DLOGOS_LIBLOGOS_ROOT=/tmp/logos-liblogos-merged \
		$(if $(NIX_QTBASE),-DCMAKE_PREFIX_PATH="$(NIX_QT_PREFIX)" -DQT_ADDITIONAL_PACKAGES_PREFIX_PATH="$(NIX_QTDECL)$$(echo ';')$(NIX_QTREMOBJ)",) \
		&& cmake --build build -j$$(nproc)
	@echo "kv_module built at: $(KV_MODULE_DIR)/build/kv_module_plugin.so"

## Install kv_module to logos-app modules dir
install-kv-module: build-kv-module
	mkdir -p ~/.local/share/Logos/LogosAppNix/modules/kv_module
	cp $(KV_MODULE_DIR)/build/kv_module_plugin.so ~/.local/share/Logos/LogosAppNix/modules/kv_module/
	echo '{"name":"kv_module","version":"0.1.0","type":"core","category":"storage","dependencies":[],"main":{"linux-x86_64":"kv_module_plugin.so","linux-aarch64":"kv_module_plugin.so","darwin-arm64":"kv_module_plugin.so"}}' > ~/.local/share/Logos/LogosAppNix/modules/kv_module/manifest.json
	@echo "kv_module installed to ~/.local/share/Logos/LogosAppNix/modules/kv_module/"

# ── delivery_module (P2P messaging backend) ───────────────────────────────────

LGPM ?= /tmp/package-manager/bin/lgpm
DELIVERY_RELEASE ?= build-20260307-a751c91-69

## Install delivery_module via lgpm
install-delivery-module:
	$(LGPM) --release $(DELIVERY_RELEASE) \
		--modules-dir ~/.local/share/Logos/LogosAppNix/modules \
		install logos-delivery-module
	@echo "delivery_module installed to ~/.local/share/Logos/LogosAppNix/modules/delivery_module/"

## Install everything: scala_ui + scala_module + kv_module + delivery_module
install-all: install install-module install-kv-module install-delivery-module
	@echo ""
	@echo "All installed! Run logos-app:"
	@echo "  cd ~/logos-workspace && nix run .#logos-app-poc"
