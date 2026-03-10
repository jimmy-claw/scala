#!/bin/bash
set -e
BUILD_DIR="${1:-build-standalone}"
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_STANDALONE=ON
make -j4 scala_standalone
cd ..
xvfb-run -a -s "-screen 0 1024x768x24" "./$BUILD_DIR/scala_standalone" &
APP_PID=$!
sleep 3
scrot screenshot.png -u || import -window root screenshot.png
kill $APP_PID 2>/dev/null || true
echo "Screenshot saved to screenshot.png"
