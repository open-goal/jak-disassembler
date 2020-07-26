cmake_minimum_required(VERSION 3.0)
project(jak2_disassembler)

set(CMAKE_CXX_STANDARD 14)

add_executable(jak2_disassembler main.cpp ObjectFileDB.cpp Disasm/Instruction.cpp Disasm/InstructionDecode.cpp Disasm/OpcodeInfo.cpp Disasm/Register.cpp LinkedObjectFileCreation.cpp LinkedObjectFile.cpp Function/Function.cpp util/FileIO.cpp)
target_include_directories(jak2_disassembler PRIVATE .)