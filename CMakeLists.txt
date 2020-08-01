cmake_minimum_required(VERSION 3.0)
project(jak_disassembler)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "-O3 -ggdb")

# Set default compile flags for GCC
if(CMAKE_COMPILER_IS_GNUCXX)
    message(STATUS "GCC detected, adding compile flags")
    set(CMAKE_CXX_FLAGS 
        "${CMAKE_CXX_FLAGS}\
        -Wall\
        -Winit-self\
        -Wextra\
        -Wcast-align\
        -Wcast-qual\
        -Wdisabled-optimization\
        -Wformat=2\
        -Wmissing-include-dirs\
        -Woverloaded-virtual\
        -Wredundant-decls\
        -Wshadow\
        -Wsign-promo")
endif(CMAKE_COMPILER_IS_GNUCXX)

add_executable(jak_disassembler
    util/LispPrint.cpp
    main.cpp
    ObjectFileDB.cpp
    Disasm/Instruction.cpp
    Disasm/InstructionDecode.cpp
    Disasm/OpcodeInfo.cpp
    Disasm/Register.cpp
    LinkedObjectFileCreation.cpp
    LinkedObjectFile.cpp
    Function/Function.cpp
    util/FileIO.cpp
    third-party/minilzo/minilzo.c
    config.cpp
    util/LispPrint.cpp
    util/Timer.cpp
    Function/BasicBlocks.cpp
    Disasm/InstructionMatching.cpp
    TypeSystem/GoalType.cpp
    TypeSystem/GoalFunction.cpp
    TypeSystem/GoalSymbol.cpp
    TypeSystem/TypeInfo.cpp
    TypeSystem/TypeSpec.cpp)

target_include_directories(jak_disassembler PRIVATE .)
