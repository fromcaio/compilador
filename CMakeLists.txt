cmake_minimum_required(VERSION 3.14)
if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.24.0")
    cmake_policy(SET CMP0135 NEW)
endif()
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

add_executable(fcc 
    src/main.cpp 
)

# Our headers live in include/
target_include_directories(fcc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Link the JSON interface target; it supplies include paths and C++ features
target_link_libraries(fcc PRIVATE nlohmann_json::nlohmann_json)