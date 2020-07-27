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

set(wxBUILD_SHARED OFF CACHE BOOL "wxBUILD_SHARED" FORCE)

set(wxUSE_STL ON CACHE BOOL "wxUSE_STL" FORCE)
set(wxUSE_GUI ON CACHE BOOL "wxUSE_GUI" FORCE)
set(wxUSE_OPENGL ON CACHE BOOL "wxUSE_OPENGL" FORCE)
set(wxUSE_WINRT	OFF CACHE BOOL "wxUSE_WINRT" FORCE)
set(wxUSE_EXCEPTIONS OFF CACHE BOOL "wxUSE_EXCEPTIONS" FORCE)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/wxWidgets" EXCLUDE_FROM_ALL)

add_executable(
	jak2_disassembler
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
    minilzo/minilzo.c
    game_version.cpp
    game_version.h
    util/LispPrint.cpp
    util/LispPrint.h
    util/Timer.cpp)
target_include_directories(jak2_disassembler PRIVATE .)

add_executable(jak_disasm_gui WIN32 gui/test.cpp)
# TODO - Compiling this on linux/mac might require some fighting
# getting per-monitor DPI scaling to work properly was not easy
# Windows seems to be the only platform that wants these manifest files
# so maybe just some CMake flags to compile on each platform? (this manifest is for 64 bit as well)
target_sources(jak_disasm_gui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/wxWidgets/include/wx/msw/amd64_dpi_aware.manifest")
target_link_libraries(jak_disasm_gui PRIVATE wxgl wxcore wxbase)
