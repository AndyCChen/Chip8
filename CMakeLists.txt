cmake_minimum_required(VERSION 3.28)
Project(chip8Interpreter)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY  ${CMAKE_CURRENT_SOURCE_DIR}/bin)

find_package(SDL2)

if(SDL2_FOUND)
   message(STATUS "SDL2 found")
endif(SDL2_FOUND)

message(STATUS ${SDL2_INCLUDE_DIRS})

add_executable(chip8 main.c src/chip8.c src/display.c src/gui.c)
target_include_directories(chip8 PRIVATE ${SDL2_INCLUDE_DIRS})

target_include_directories(chip8 INTERFACE ./nuklear)
target_link_libraries(chip8 PRIVATE SDL2::SDL2main SDL2::SDL2)