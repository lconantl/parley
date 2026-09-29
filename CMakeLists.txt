cmake_minimum_required(VERSION 3.20)
project(parley VERSION 2.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(CURL REQUIRED)
find_package(SQLite3 REQUIRED)
find_package(nlohmann_json 3.10 REQUIRED)
find_package(Threads REQUIRED)

file(GLOB_RECURSE CORE_FILES CONFIGURE_DEPENDS "src/*.cpp")
list(REMOVE_ITEM CORE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")
add_library(core STATIC ${CORE_FILES})
target_include_directories(core PUBLIC src)
target_link_libraries(core PUBLIC CURL::libcurl SQLite::SQLite3
    nlohmann_json::nlohmann_json Threads::Threads)
if(MSVC)
    target_compile_options(core PUBLIC /W4 /utf-8 /permissive-)
    target_compile_definitions(core PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
else()
    target_compile_options(core PUBLIC -Wall -Wextra -Wpedantic -Werror)
endif()

add_executable(parley src/main.cpp)
target_link_libraries(parley PRIVATE core)

include(CTest)
if(BUILD_TESTING)
    find_package(GTest REQUIRED)
    file(GLOB_RECURSE TEST_FILES CONFIGURE_DEPENDS "tests/*.cpp")
    add_executable(parley_tests ${TEST_FILES})
    target_link_libraries(parley_tests PRIVATE core GTest::gtest_main)
    target_compile_definitions(parley_tests PRIVATE
        PARLEY_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")
    include(GoogleTest)
    gtest_discover_tests(parley_tests)
endif()
