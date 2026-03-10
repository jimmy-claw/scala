BUILD_DIR ?= build
CMAKE_FLAGS ?= -DCMAKE_BUILD_TYPE=Debug

.PHONY: all clean build test

all: build

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

build: $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. $(CMAKE_FLAGS) && make -j$$(nproc)

test: $(BUILD_DIR)
	cd $(BUILD_DIR) && cmake .. $(CMAKE_FLAGS) -DBUILD_TESTS=ON && make -j$$(nproc) && ctest --output-on-failure

clean:
	rm -rf $(BUILD_DIR)
