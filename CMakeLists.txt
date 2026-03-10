cmake_minimum_required(VERSION 3.16)
project(scala VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── Qt6 ───────────────────────────────────────────────────────────────────────
find_package(Qt6 REQUIRED COMPONENTS Core Qml)

set(CMAKE_AUTOMOC ON)

# ── Logos SDK + liblogos ──────────────────────────────────────────────────────
set(LOGOS_CPP_SDK_ROOT "" CACHE PATH "Path to logos-cpp-sdk install")
set(LOGOS_LIBLOGOS_ROOT "" CACHE PATH "Path to logos-liblogos install")

if(NOT "${LOGOS_CPP_SDK_ROOT}" STREQUAL "" AND NOT "${LOGOS_LIBLOGOS_ROOT}" STREQUAL "")
    set(LOGOS_CORE_AVAILABLE ON)

    add_library(logos_cpp_sdk INTERFACE)
    target_include_directories(logos_cpp_sdk INTERFACE
        "${LOGOS_LIBLOGOS_ROOT}/include"
        "${LOGOS_CPP_SDK_ROOT}/include"
        "${LOGOS_CPP_SDK_ROOT}/include/cpp"
        "${LOGOS_CPP_SDK_ROOT}/include/core"
    )

    add_library(logos_core SHARED IMPORTED)
    set_target_properties(logos_core PROPERTIES
        IMPORTED_LOCATION "${LOGOS_LIBLOGOS_ROOT}/lib/liblogos_core.so"
    )

    find_library(LOGOS_SDK_LIB logos_sdk PATHS "${LOGOS_CPP_SDK_ROOT}/lib" NO_DEFAULT_PATH REQUIRED)
endif()

# ── Plugin target ─────────────────────────────────────────────────────────────
qt_add_plugin(scala_plugin CLASS_NAME LogosCalendar)
set_target_properties(scala_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "scala_plugin"
)

target_sources(scala_plugin PRIVATE
    src/calendar_module.cpp
    src/calendar_store.cpp
    src/plugin.cpp
)

target_include_directories(scala_plugin PRIVATE src ${CMAKE_SOURCE_DIR})

target_link_libraries(scala_plugin PRIVATE
    Qt6::Core
    Qt6::Qml
)

if(LOGOS_CORE_AVAILABLE)
    find_package(Qt6 REQUIRED COMPONENTS RemoteObjects)
    target_link_libraries(scala_plugin PRIVATE
        Qt6::RemoteObjects
        logos_cpp_sdk
        logos_core
        ${LOGOS_SDK_LIB}
    )
    target_compile_definitions(scala_plugin PRIVATE LOGOS_CORE_AVAILABLE)
endif()

# ── Compile definitions ──────────────────────────────────────────────────────
target_compile_definitions(scala_plugin PRIVATE
    QT_PLUGIN
)

# ── RPATH ─────────────────────────────────────────────────────────────────────
if(APPLE)
    set_target_properties(scala_plugin PROPERTIES
        BUILD_RPATH   "@loader_path"
        INSTALL_RPATH "@loader_path"
    )
elseif(UNIX)
    set_target_properties(scala_plugin PROPERTIES
        BUILD_RPATH   "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
    )
endif()

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS scala_plugin
    LIBRARY DESTINATION lib/scala
)

install(FILES ${CMAKE_SOURCE_DIR}/metadata.json
    DESTINATION lib/scala
    RENAME manifest.json
)

# ── Tests ─────────────────────────────────────────────────────────────────────
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
