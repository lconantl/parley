cmake_minimum_required(VERSION 3.28)

if (POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif ()

project(parley VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(TgBot CONFIG REQUIRED)
find_package(httplib CONFIG REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)
find_package(libzip CONFIG REQUIRED)
find_package(unofficial-libharu CONFIG REQUIRED)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS "src/*.cpp")
list(REMOVE_ITEM SRC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cpp")

add_library(core STATIC ${SRC_FILES})

target_include_directories(core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)
target_link_libraries(core PUBLIC
        TgBot::TgBot
        httplib::httplib
        nlohmann_json::nlohmann_json
        libzip::zip
        unofficial::libharu::hpdf
)

if (MSVC)
    target_compile_options(core PRIVATE /W4 /utf-8)
    target_compile_definitions(core PUBLIC _WIN32_WINNT=0x0A00)
else ()
    target_compile_options(core PRIVATE -Wall -Wextra -Wpedantic -Werror)
endif ()

if (WIN32)
    target_compile_definitions(core PUBLIC _WIN32_WINNT=0x0A00)
endif ()

add_executable(parley src/main.cpp)
target_link_libraries(parley PRIVATE core)