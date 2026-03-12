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
qt_add_plugin(scala_module_plugin CLASS_NAME LogosCalendar)
set_target_properties(scala_module_plugin PROPERTIES
    PREFIX ""
    OUTPUT_NAME "scala_module_plugin"
)

target_sources(scala_module_plugin PRIVATE
    src/calendar_module.cpp
    src/calendar_store.cpp
    src/calendar_sync.cpp
    src/qr_generator.cpp
    src/plugin.cpp
)

target_include_directories(scala_module_plugin PRIVATE src ${CMAKE_SOURCE_DIR})

target_link_libraries(scala_module_plugin PRIVATE
    Qt6::Core
    Qt6::Qml
)

if(LOGOS_CORE_AVAILABLE)
    find_package(Qt6 REQUIRED COMPONENTS RemoteObjects)
    target_link_libraries(scala_module_plugin PRIVATE
        Qt6::RemoteObjects
        logos_cpp_sdk
        logos_core
        ${LOGOS_SDK_LIB}
    )
    target_compile_definitions(scala_module_plugin PRIVATE LOGOS_CORE_AVAILABLE)
endif()

# ── Compile definitions ──────────────────────────────────────────────────────
target_compile_definitions(scala_module_plugin PRIVATE
    QT_PLUGIN
)

# ── RPATH ─────────────────────────────────────────────────────────────────────
if(APPLE)
    set_target_properties(scala_module_plugin PROPERTIES
        BUILD_RPATH   "@loader_path"
        INSTALL_RPATH "@loader_path"
    )
elseif(UNIX)
    set_target_properties(scala_module_plugin PROPERTIES
        BUILD_RPATH   "$ORIGIN"
        INSTALL_RPATH "$ORIGIN"
    )
endif()

# ── Install ───────────────────────────────────────────────────────────────────
install(TARGETS scala_module_plugin
    LIBRARY DESTINATION lib/scala
)

install(FILES ${CMAKE_SOURCE_DIR}/metadata.json
    DESTINATION lib/scala
    RENAME manifest.json
)

# ── Standalone runner ────────────────────────────────────────────────────────
option(BUILD_STANDALONE "Build standalone runner" OFF)
if(BUILD_STANDALONE)
    find_package(Qt6 REQUIRED COMPONENTS Quick Gui Widgets)

    qt_add_executable(scala_standalone standalone/main.cpp)

    target_sources(scala_standalone PRIVATE
        src/calendar_module.cpp
        src/calendar_store.cpp
        src/calendar_sync.cpp
        src/qr_generator.cpp
    )

    target_include_directories(scala_standalone PRIVATE src)

    qt_add_qml_module(scala_standalone
        URI "ScalaApp"
        VERSION 1.0
        QML_FILES
            qml/CalendarView.qml
            qml/CalendarGrid.qml
            qml/CalendarSidebar.qml
            qml/EventModal.qml
            qml/EventDetails.qml
            qml/ShareDialog.qml
            qml/SettingsPanel.qml
        RESOURCE_PREFIX "/ScalaApp"
    )

    target_link_libraries(scala_standalone PRIVATE
        Qt6::Quick Qt6::Qml Qt6::Gui Qt6::Widgets Qt6::RemoteObjects
    )

    if(LOGOS_CORE_AVAILABLE)
        target_sources(scala_standalone PRIVATE src/scala_bridge.cpp)
        target_link_libraries(scala_standalone PRIVATE
            logos_cpp_sdk
            logos_core
            ${LOGOS_SDK_LIB}
            Qt6::RemoteObjects
        )
        target_compile_definitions(scala_standalone PRIVATE LOGOS_CORE_AVAILABLE)
    endif()
endif()

# ── Headless logoscore module plugin ─────────────────────────────────────────
option(BUILD_MODULE "Build headless logoscore plugin (no Qt Quick/GUI)" OFF)
if(BUILD_MODULE)
    if(NOT LOGOS_CPP_SDK_ROOT OR NOT LOGOS_LIBLOGOS_ROOT)
        message(FATAL_ERROR "BUILD_MODULE requires -DLOGOS_CPP_SDK_ROOT=<path> -DLOGOS_LIBLOGOS_ROOT=<path>")
    endif()

    find_package(Qt6 REQUIRED COMPONENTS Core Qml RemoteObjects)

    qt_add_plugin(scala_module_plugin CLASS_NAME ScalaPlugin)
    set_target_properties(scala_module_plugin PROPERTIES
        PREFIX ""
        OUTPUT_NAME "scala_module_plugin"
    )

    target_sources(scala_module_plugin PRIVATE
        src/scala_plugin.cpp
        src/scala_plugin.h
        src/calendar_module.cpp
        src/calendar_store.cpp
        src/calendar_sync.cpp
        src/qr_generator.cpp
    )

    target_include_directories(scala_module_plugin PRIVATE
        src
        ${CMAKE_SOURCE_DIR}
        "${LOGOS_LIBLOGOS_ROOT}/include"
        "${LOGOS_CPP_SDK_ROOT}/include"
        "${LOGOS_CPP_SDK_ROOT}/include/cpp"
        "${LOGOS_CPP_SDK_ROOT}/include/core"
    )

    # Logos libraries
    find_library(MODULE_LOGOS_CORE_LIB logos_core
        HINTS "${LOGOS_LIBLOGOS_ROOT}/lib" NO_DEFAULT_PATH)
    target_link_libraries(scala_module_plugin PRIVATE
        Qt6::Core Qt6::Qml Qt6::RemoteObjects
        ${MODULE_LOGOS_CORE_LIB}
    )

    target_compile_definitions(scala_module_plugin PRIVATE
        QT_PLUGIN
        LOGOS_CORE_AVAILABLE
        SCALA_MODULE_WRAPPER
    )

    # RPATH: $ORIGIN so it finds logos libs next to the .so
    if(APPLE)
        set_target_properties(scala_module_plugin PROPERTIES
            BUILD_RPATH   "@loader_path"
            INSTALL_RPATH "@loader_path"
        )
    elseif(UNIX)
        set_target_properties(scala_module_plugin PROPERTIES
            BUILD_RPATH   "$ORIGIN"
            INSTALL_RPATH "$ORIGIN"
        )
    endif()
endif()

# ── UI Plugin (IComponent for logos-app-poc) ─────────────────────────────────
option(BUILD_UI_PLUGIN "Build the IComponent UI plugin for logos-app-poc" OFF)

if(BUILD_UI_PLUGIN)
    find_package(Qt6 REQUIRED COMPONENTS Widgets Quick QuickWidgets)

    # component-interfaces (IComponent.h)
    find_package(component-interfaces QUIET)
    if(NOT component-interfaces_FOUND)
        add_library(component-interfaces INTERFACE)
        target_include_directories(component-interfaces INTERFACE
            "${CMAKE_CURRENT_SOURCE_DIR}/interfaces"
        )
    endif()

    set(UI_TARGET scala_ui)

    add_library(${UI_TARGET} SHARED
        src/scala_ui_component.cpp
        src/scala_plugin.cpp
        src/calendar_module.cpp
        src/calendar_store.cpp
        src/calendar_sync.cpp
        src/qr_generator.cpp
        qml/scala_ui.qrc
    )

    target_include_directories(${UI_TARGET} PRIVATE
        src
        ${CMAKE_SOURCE_DIR}
    )

    target_compile_definitions(${UI_TARGET} PRIVATE
        SCALA_UI_BUILD
        SCALA_UI_METADATA_FILE="${CMAKE_CURRENT_SOURCE_DIR}/ui_metadata.json"
    )

    target_link_libraries(${UI_TARGET} PRIVATE
        Qt6::Core
        Qt6::Widgets
        Qt6::Quick
        Qt6::Qml
        Qt6::QuickWidgets
        component-interfaces
    )

    if(LOGOS_CORE_AVAILABLE)
        find_package(Qt6 REQUIRED COMPONENTS RemoteObjects)
        target_link_libraries(${UI_TARGET} PRIVATE
            logos_cpp_sdk
            logos_core
            ${LOGOS_SDK_LIB}
            Qt6::RemoteObjects
        )
        target_compile_definitions(${UI_TARGET} PRIVATE LOGOS_CORE_AVAILABLE)
    endif()

    if(APPLE)
        set_target_properties(${UI_TARGET} PROPERTIES
            BUILD_RPATH   "@loader_path"
            INSTALL_RPATH "@loader_path"
        )
    elseif(UNIX)
        set_target_properties(${UI_TARGET} PROPERTIES
            BUILD_RPATH   "$ORIGIN"
            INSTALL_RPATH "$ORIGIN"
        )
    endif()

    install(TARGETS ${UI_TARGET}
        LIBRARY DESTINATION lib
    )
endif()

# ── Tests ─────────────────────────────────────────────────────────────────────
option(BUILD_TESTS "Build unit tests" OFF)
if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
