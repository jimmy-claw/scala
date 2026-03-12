cmake_minimum_required(VERSION 3.16)

if(NOT CMAKE_PROJECT_NAME OR CMAKE_PROJECT_NAME STREQUAL "scala_tests")
    project(scala_tests LANGUAGES CXX)
    set(CMAKE_CXX_STANDARD 17)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Test)

set(CMAKE_AUTOMOC ON)

enable_testing()

add_executable(test_calendar
    test_calendar.cpp
)

target_include_directories(test_calendar PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
)

target_link_libraries(test_calendar PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME test_calendar COMMAND test_calendar)

# ── Sync tests ───────────────────────────────────────────────────────────────

add_executable(test_sync
    test_sync.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/calendar_sync.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/calendar_store.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/calendar_module.cpp
)

target_include_directories(test_sync PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(test_sync PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME test_sync COMMAND test_sync)

# ── Sharing tests ────────────────────────────────────────────────────────────

add_executable(test_sharing
    test_sharing.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/calendar_module.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/calendar_sync.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/calendar_store.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/qr_generator.cpp
)

target_include_directories(test_sharing PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(test_sharing PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME test_sharing COMMAND test_sharing)

# ── Identity tests ────────────────────────────────────────────────────────────

add_executable(test_identity
    test_identity.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/calendar_module.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/calendar_sync.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/calendar_store.cpp
)

target_include_directories(test_identity PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(test_identity PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME test_identity COMMAND test_identity)

# ── Module wiring tests ──────────────────────────────────────────────────────

add_executable(test_module
    test_module.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/calendar_module.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/calendar_sync.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/calendar_store.cpp
)

target_include_directories(test_module PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(test_module PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME test_module COMMAND test_module)

# ── Namespace isolation tests ─────────────────────────────────────────────────

add_executable(test_namespace
    test_namespace.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/calendar_store.cpp
)

target_include_directories(test_namespace PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(test_namespace PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME test_namespace COMMAND test_namespace)

# ── QML UI tests (QQuickTest) ───────────────────────────────────────────────

find_package(Qt6 COMPONENTS QuickTest Quick QUIET)
if(Qt6QuickTest_FOUND)
    qt_add_executable(test_qml qml/tst_main.cpp)
    target_link_libraries(test_qml PRIVATE Qt6::QuickTest Qt6::Quick)
    qt_add_resources(test_qml "qml_tests"
        PREFIX "/"
        FILES qml/tst_CalendarView.qml
    )
    add_test(NAME test_qml
        COMMAND test_qml -input ${CMAKE_CURRENT_SOURCE_DIR}/qml
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    )
    set_tests_properties(test_qml PROPERTIES
        ENVIRONMENT "QT_QPA_PLATFORM=offscreen"
    )
else()
    message(STATUS "Qt6::QuickTest not found — skipping QML tests (install qt6-declarative-dev)")
endif()

# ── Encryption tests ──────────────────────────────────────────────────────────

add_executable(test_encryption
    test_encryption.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/calendar_module.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/calendar_sync.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/calendar_store.cpp
)

target_include_directories(test_encryption PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../src
    ${CMAKE_CURRENT_SOURCE_DIR}/..
)

target_link_libraries(test_encryption PRIVATE
    Qt6::Core
    Qt6::Test
)

add_test(NAME test_encryption COMMAND test_encryption)
