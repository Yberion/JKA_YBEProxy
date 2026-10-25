# Based on https://github.com/cpp-best-practices/cmake_template (see ProjectOptions.cmake and cmake/)
cmake_minimum_required(VERSION 3.25)

# gnu++20 and not c++20: the code copied from the engine uses GNU extensions
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

project(
  JKA_YBEProxy
  DESCRIPTION "Proxy game module for the Jedi Academy dedicated server"
  HOMEPAGE_URL "https://github.com/Yberion/JKA_YBEProxy"
  LANGUAGES CXX C)

if(NOT WIN32 AND NOT CMAKE_SYSTEM_NAME STREQUAL "Linux")
  message(FATAL_ERROR "Only Windows and Linux are supported")
endif()

include(cmake/PreventInSourceBuilds.cmake)
include(ProjectOptions.cmake)

ybeproxy_setup_options()
ybeproxy_global_options()
ybeproxy_local_options()

set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# Must match ARCH_STRING in sdk/qcommon/q_platform.hpp, and gives the module name (jampgamex86.dll, jampgamei386.so)
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
  set(YBEPROXY_ARCH "x86_64")
elseif(WIN32)
  set(YBEPROXY_ARCH "x86")
else()
  set(YBEPROXY_ARCH "i386")
endif()
message(STATUS "Architecture is ${YBEPROXY_ARCH}")

# Zydis, the disassembler HookUtils uses to measure and check the instructions it copies (git submodule, see README)
# Static library, decoder only, built like the module: position independent code, hidden symbols, static MSVC runtime
if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/external/zydis/dependencies/zycore/CMakeLists.txt")
  message(FATAL_ERROR "Zydis is missing: run 'git submodule update --init --recursive'")
endif()
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
foreach(option ZYDIS_BUILD_SHARED_LIB ZYDIS_BUILD_EXAMPLES ZYDIS_BUILD_TOOLS ZYDIS_BUILD_TESTS ZYDIS_BUILD_DOXYGEN
                ZYDIS_FEATURE_ENCODER ZYDIS_FEATURE_FORMATTER ZYDIS_FEATURE_AVX512 ZYDIS_FEATURE_KNC ZYDIS_FEATURE_SEGMENT)
  set(${option} OFF CACHE BOOL "" FORCE)
endforeach()
set(ZYDIS_FEATURE_DECODER ON CACHE BOOL "" FORCE)
add_subdirectory(external/zydis EXCLUDE_FROM_ALL SYSTEM)

add_subdirectory(src)
