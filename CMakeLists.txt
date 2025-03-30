cmake_minimum_required(VERSION 3.10)
project(RetroConsole)

set(CMAKE_CXX_STANDARD 17)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(retro_console 
    src/main.cpp 
    src/ui.cpp 
    src/emulator_launcher.cpp
)

target_link_libraries(retro_console ${SDL2_LIBRARIES})
