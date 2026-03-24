cmake_minimum_required(VERSION 3.14)
project(Bootloader)

set(CMAKE_CXX_STANDARD 17)

# --- Dependency Management ---
include(FetchContent)

# Download the JSON library release archive (official recommendation)
set(JSON_BuildTests OFF CACHE INTERNAL "") # we don't need the library's tests

FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
)
FetchContent_MakeAvailable(json)

# --- Project Setup ---
include_directories(include)

add_executable(compilador 
    src/main.cpp 
)

# Our headers live in include/
target_include_directories(compilador PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link the JSON interface target; it supplies include paths and C++ features
target_link_libraries(compilador PRIVATE nlohmann_json::nlohmann_json)