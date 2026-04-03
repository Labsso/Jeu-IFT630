cmake_minimum_required(VERSION 3.16)
project(Jeu_IFT630)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Generate compile_commands.json (useful for CLion tooling)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---- Raylib ----
include(FetchContent)

set(RAYLIB_VERSION 5.5)

FetchContent_Declare(
        raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/${RAYLIB_VERSION}.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
)

FetchContent_MakeAvailable(raylib)

# ---- Your executable ----
add_executable(Jeu_IFT630
    main.cpp
    globals.cpp
    render.cpp
    threads.cpp
)

# Link raylib
target_link_libraries(Jeu_IFT630 raylib)