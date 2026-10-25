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
  LANGUAGES CXX)

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

add_subdirectory(src)
