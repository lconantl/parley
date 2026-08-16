cmake_minimum_required(VERSION 3.28)
project(parley VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(TgBot REQUIRED)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "src/*.cpp")
list(REMOVE_ITEM SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

add_library(core STATIC ${SRC_FILES})
target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

target_compile_options(core PRIVATE -Wall -Wextra -Wpedantic -Werror)

add_executable(parley src/main.cpp)
target_link_libraries(parley PRIVATE core TgBot)
set_target_properties(parley PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/res")