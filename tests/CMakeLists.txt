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
