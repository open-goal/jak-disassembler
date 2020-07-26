cmake_minimum_required(VERSION 3.0)
project(jak2_disassembler)

set(CMAKE_CXX_STANDARD 14)

set(CMAKE_CXX_FLAGS "-O3 -ggdb -Wall \
-Wextra -Wcast-align -Wcast-qual -Wdisabled-optimization -Wformat=2 \
-Winit-self -Wmissing-include-dirs -Woverloaded-virtual \
-Wredundant-decls -Wshadow -Wsign-promo ")

add_executable(jak2_disassembler util/LispPrint.cpp main.cpp ObjectFileDB.cpp Disasm/Instruction.cpp Disasm/InstructionDecode.cpp Disasm/OpcodeInfo.cpp Disasm/Register.cpp LinkedObjectFileCreation.cpp LinkedObjectFile.cpp Function/Function.cpp util/FileIO.cpp minilzo/minilzo.c game_version.cpp game_version.h util/LispPrint.cpp util/LispPrint.h)
target_include_directories(jak2_disassembler PRIVATE .)