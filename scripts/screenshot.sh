#!/bin/bash
set -e
BUILD_DIR="${1:-build-standalone}"
mkdir -p "$BUILD_DIR" && cd "$BUILD_DIR"
cmake .. -DCMAKE_BUILD_TYPE=Debug -DBUILD_STANDALONE=ON
make -j4 scala_standalone
cd ..

# Kill any existing Xvfb on :20
kill $(cat /tmp/.xvfb-20.pid 2>/dev/null) 2>/dev/null || true
Xvfb :20 -screen 0 1024x768x24 &
echo $! > /tmp/.xvfb-20.pid
sleep 1

DISPLAY=:20 QT_QUICK_BACKEND=software LIBGL_ALWAYS_SOFTWARE=1 \
    ./$BUILD_DIR/scala_standalone &
APP_PID=$!
sleep 4

DISPLAY=:20 scrot screenshot.png -u
kill $APP_PID 2>/dev/null || true
kill $(cat /tmp/.xvfb-20.pid) 2>/dev/null || true
echo "Screenshot saved: screenshot.png"
