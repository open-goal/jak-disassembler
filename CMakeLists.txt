cmake_minimum_required(VERSION 3.0)
project(jak2_disassembler)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "-O3 -ggdb")

# Set default compile flags for GCC
if(CMAKE_COMPILER_IS_GNUCXX)
    message(STATUS "GCC detected, adding compile flags")
    set(CMAKE_CXX_FLAGS 
        "${CMAKE_CXX_FLAGS} 
        -Wall-Winit-self 
        -Wextra
        -Wcast-align
        -Wcast-qual
        -Wdisabled-optimization
        -Wformat=2
        -Wmissing-include-dirs
        -Woverloaded-virtual
        -Wredundant-decls
        -Wshadow
        -Wsign-promo")
endif(CMAKE_COMPILER_IS_GNUCXX)

add_executable(jak2_disassembler
    util/LispPrint.cpp main.cpp
    ObjectFileDB.cpp
    Disasm/Instruction.cpp
    Disasm/InstructionDecode.cpp
    Disasm/OpcodeInfo.cpp
    Disasm/Register.cpp
    LinkedObjectFileCreation.cpp
    LinkedObjectFile.cpp
    Function/Function.cpp
    util/FileIO.cpp
    minilzo/minilzo.c
    game_version.cpp
    game_version.h
    util/LispPrint.cpp
    util/LispPrint.h
    util/Timer.cpp)
target_include_directories(jak2_disassembler PRIVATE .)