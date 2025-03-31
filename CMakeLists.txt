cmake_minimum_required(VERSION 3.10)
project(RetroConsole)

set(CMAKE_CXX_STANDARD 17)

# Find pkg-config
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)
pkg_check_modules(SDL2_IMAGE REQUIRED SDL2_image)
pkg_check_modules(SDL2_TTF REQUIRED SDL2_ttf)

# Find CURL
find_package(CURL REQUIRED)

# Include nlohmann/json
include(FetchContent)
FetchContent_Declare(
    json
    URL https://github.com/nlohmann/json/releases/download/v3.11.3/json.tar.xz
)
FetchContent_MakeAvailable(json)

include_directories(${SDL2_INCLUDE_DIRS} ${SDL2_IMAGE_INCLUDE_DIRS} ${SDL2_TTF_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS})
link_directories(${SDL2_LIBRARY_DIRS} ${SDL2_IMAGE_LIBRARY_DIRS} ${SDL2_TTF_LIBRARY_DIRS})

add_executable(retro_console 
    src/main.cpp 
    src/sdl_ui.cpp
    src/emulator_launcher.cpp
    src/igdb_client.cpp
    src/save_manager.cpp
)

target_link_libraries(retro_console 
    ${SDL2_LIBRARIES} 
    ${SDL2_IMAGE_LIBRARIES} 
    ${SDL2_TTF_LIBRARIES}
    ${CURL_LIBRARIES}
    nlohmann_json::nlohmann_json
)