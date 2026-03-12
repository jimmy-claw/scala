#!/bin/bash
# start-scala-with-kv.sh - Launch Scala with direct kv_module support (no logos_host needed)
#
# This script demonstrates how to run Scala with direct kv_module plugin linkage,
# which provides persistence WITHOUT requiring logos_host to be running.
#
# Requirements:
#   - logos-kv-module must be built (builds libkv_module_plugin.so)
#   - kv_module_plugin.so must be in a known location (e.g., /path/to/kv_module/lib/)
#
# Usage:
#   ./start-scala-with-kv.sh [/path/to/kv_module/lib]
#
# Example:
#   ./start-scala-with-kv.sh ~/lssa/kv-module/build/lib
#
# If no path is given, it tries to find kv_module_plugin.so in common locations.

set -e

# Optional: path to kv_module library directory
KV_MODULE_PATH="${1:-}"

if [[ -z "$KV_MODULE_PATH" ]]; then
    # Auto-detect kv_module
    echo "Auto-detecting kv_module plugin..."
    
    # Try common locations
    for loc in \
        "/usr/local/lib" \
        "/usr/lib" \
        "$HOME/.local/lib" \
        "$HOME/lssa/kv-module/build/lib" \
        "$HOME/openclaw/workspace/logos-kv-module/build/lib" \
        "$HOME/.openclaw/workspace/logos-kv-module/build/lib"; do
        
        if [[ -f "$loc/libkv_module_plugin.so" ]]; then
            KV_MODULE_PATH="$loc"
            echo "Found kv_module at: $KV_MODULE_PATH"
            break
        fi
    done
    
    if [[ -z "$KV_MODULE_PATH" ]]; then
        echo "Error: kv_module_plugin.so not found!"
        echo "Please build logos-kv-module first, then run:"
        echo "  ./start-scala-with-kv.sh /path/to/kv_module/lib"
        exit 1
    fi
fi

# Make script executable
chmod +x "$(dirname "$0")/start-scala-with-kv.sh"

# Set RPATH so the standalone can find kv_module
export LD_LIBRARY_PATH="$KV_MODULE_PATH:$LD_LIBRARY_PATH"

# Build Scala with kv_module support
cd "$(dirname "$0")/.."
echo "Building Scala with direct kv_module support..."

# Clean and reconfigure
rm -rf build
cmake -B build \
    -DCMAKE_BUILD_TYPE=Debug \
    -DCMAKE_PREFIX_PATH=/usr/lib/qt6 \
    -DBUILD_STANDALONE=ON \
    -DKV_MODULE_INCLUDE_DIR="${KV_MODULE_PATH}/../include" \
    -DKV_MODULE_AVAILABLE=ON

cmake --build build

# Run
echo "Starting Scala Calendar (with kv_module persistence)..."
./build/scala_standalone
