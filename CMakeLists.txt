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

set(wxBUILD_SHARED OFF CACHE BOOL "wxBUILD_SHARED" FORCE)

set(wxUSE_STL ON CACHE BOOL "wxUSE_STL" FORCE)
set(wxUSE_GUI ON CACHE BOOL "wxUSE_GUI" FORCE)
set(wxUSE_OPENGL ON CACHE BOOL "wxUSE_OPENGL" FORCE)
set(wxUSE_WINRT	OFF CACHE BOOL "wxUSE_WINRT" FORCE)
set(wxUSE_EXCEPTIONS OFF CACHE BOOL "wxUSE_EXCEPTIONS" FORCE)

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/wxWidgets" EXCLUDE_FROM_ALL)

set(commonSources
	util/LispPrint.cpp
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
    Function/BasicBlocks.cpp
    Disasm/InstructionMatching.cpp
    TypeSystem/GoalType.cpp
    TypeSystem/GoalFunction.cpp
    TypeSystem.GoalSymbol.cpp
    TypeSystem/TypeInfo.cpp
    TypeSystem/TypeSpec.cpp
    game_version.cpp
    util/LispPrint.cpp
    util/LispPrint.h
    util/Timer.cpp
	Api.cpp)

set(commonHeaders
	LinkedObjectFile.h
	Disasm/Instruction.h
	util/Log.h
	game_version.h
	Api.h)

add_executable(
	jak2_disassembler
    ${commonSources}
	${commonHeaders}
	main.cpp)
target_include_directories(jak2_disassembler PRIVATE .)

add_executable(jak_disasm_gui WIN32
	${commonSources}
	${commonHeaders}
	gui/App.cpp
	gui/App.h
	gui/MainFrame.cpp
	gui/MainFrame.h)
# TODO - Compiling this on linux/mac might require some fighting
# getting per-monitor DPI scaling to work properly was not easy
# Windows seems to be the only platform that wants these manifest files
# so maybe just some CMake flags to compile on each platform? (this manifest is for 64 bit as well)
target_sources(jak_disasm_gui PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/wxWidgets/include/wx/msw/amd64_dpi_aware.manifest")
target_link_libraries(jak_disasm_gui PRIVATE wxgl wxcore wxbase)
target_include_directories(jak_disasm_gui PRIVATE .)
