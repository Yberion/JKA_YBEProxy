#============================================================================
# Copyright (C) 2013 - 2018, OpenJK contributors
# 
# This file is part of the OpenJK source code.
# 
# OpenJK is free software; you can redistribute it and/or modify it
# under the terms of the GNU General Public License version 2 as
# published by the Free Software Foundation.
# 
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
# 
# You should have received a copy of the GNU General Public License
# along with this program; if not, see <http://www.gnu.org/licenses/>.
#============================================================================

cmake_minimum_required(VERSION 3.1)

# For checks in subdirectories
set(InJKA_YBEProxy TRUE)

# Project name
set(ProjectName "JKA_YBEProxy" CACHE STRING "Project Name")
project(${ProjectName})

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

option(BuildJKA_YBEProxy "Whether to create projects for the JKA_YBEProxy library (jampgame)" ON)

# Configure the use of bundled libraries.  By default, we assume the user is on
# a platform that does not require any bundling.
#
# Note that we always use the bundled copy of minizip, since it is modified to
# use Z_Malloc.

if(CMAKE_SYSTEM_NAME MATCHES "BSD")
  add_definitions(-DIOAPI_NO_64)
endif()

# Custom CMake Modules needed
list(INSERT CMAKE_MODULE_PATH 0 "${CMAKE_SOURCE_DIR}/CMakeModules")

Include(CheckTypeSize)
check_type_size("void*" CMAKE_SIZEOF_VOID_P)

# ${Architecture} must match ARCH_STRING in q_platform.h,
# and is used in DLL names (jagamex86.dll, jagamex86.dylib, jagamei386.so).
if(WIN32)
	set(X86 ON)
	if(CMAKE_SIZEOF_VOID_P MATCHES "8")
		set(Architecture "x86_64")
		set(WIN64 TRUE)
	else()
		set(Architecture "x86")
		set(WIN64 FALSE)
	endif()
else()
	set(X86 OFF)
	if(CMAKE_SYSTEM_PROCESSOR MATCHES "^arm")
		set(Architecture "arm")
	elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^i.86$")
		set(X86 ON)
		if(APPLE)
			set(Architecture "x86")
		else()
			# e.g. Linux
			set(Architecture "i386")
		endif()
	elseif(CMAKE_SYSTEM_PROCESSOR MATCHES "^(x86.64|amd64)$")
		# 64bits arch but compiling 32bits app
		if("${TARGET_ARCH}" MATCHES "x86")
			set(X86 ON)
			set(Architecture "i386")

			# set -m32 on Clang and GCC
			if(("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU") OR ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang"))
				set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
			elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "Intel")
				set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mia32")
				set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mia32")
			endif()
		else()
			set(Architecture "x86_64")
		endif()
	elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "powerpc")
		set(Architecture "ppc")
	elseif(CMAKE_SYSTEM_PROCESSOR STREQUAL "powerpc64")
		set(Architecture "ppc64")
	else()
		set(Architecture "${CMAKE_SYSTEM_PROCESSOR}")
	endif()
endif()

message(STATUS "Architecture is ${Architecture}")

if(WIN32 AND CMAKE_VERSION VERSION_LESS "3.4")
	message(WARNING "Building on Windows platform with CMake version less than 3.4 is deprecated. Manifest file will fail to be included.")
endif()

# Current Git SHA1 hash
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)
message(STATUS "Git revision is ${GIT_SHA1}")

# Binary names
set(JKA_YBEProxy "jampgame${Architecture}")

# Paths
set(JKA_YBEProxyDir "${CMAKE_SOURCE_DIR}/src")

# Operating settings
if(WIN64)
	set(SharedDefines ${SharedDefines} "WIN64")
endif()

if (APPLE)
	set(SharedDefines "MACOS_X")
endif()

if (NOT WIN32 AND NOT APPLE)
	set(SharedDefines "ARCH_STRING=\"${Architecture}\"")
endif()

# Compiler settings
if(MSVC)
	set(SharedDefines ${SharedDefines} "NOMINMAX")
	set(SharedDefines ${SharedDefines} "_CRT_SECURE_NO_WARNINGS")
	set(SharedDefines ${SharedDefines} "_SCL_SECURE_NO_WARNINGS")
	set(SharedDefines ${SharedDefines} "_CRT_NONSTDC_NO_DEPRECATE")

	if (NOT WIN64)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /arch:SSE2")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /arch:SSE2")
	endif()

	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /EHsc /sdl /Ot /Gy /TC")
	
	# Configure MSVC Runtime
	include(MSVCRuntime)
	if(NOT DEFINED MSVC_RUNTIME)
		set(MSVC_RUNTIME "dynamic")
	endif()
	configure_msvc_runtime()

	# We don't try to control symbol visibility under MSVC.
	set(JKA_YBEProxy_VISIBILITY_FLAGS "")

elseif (("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU") OR ("${CMAKE_C_COMPILER_ID}" STREQUAL "Intel") OR ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang"))
	# I hope this doesn't come back to bite me in the butt later on.
	# Realistically though, can the C and CXX compilers be different?

	# Visibility can't be set project-wide -- it needs to be specified on a
	# per-target basis.  This is primarily due to the bundled copy of ZLib.
	# ZLib explicitly declares symbols hidden, rather than defaulting to hidden.
	#
	# Note that -fvisibility=hidden is stronger than -fvisibility-inlines-hidden.
	set(JKA_YBEProxy_VISIBILITY_FLAGS "-fvisibility=hidden")

	# removes the -rdynamic flag at linking (which causes crashes for some reason)
	set(CMAKE_SHARED_LIBRARY_LINK_C_FLAGS "")
	set(CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "")

	# additional flags for debug configuration
	set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -ggdb")
	set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb")

	if (X86)
		if (("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU") OR ("${CMAKE_C_COMPILER_ID}" MATCHES "Clang"))
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -msse2")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -msse2")
		endif()
	endif()

	set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3") 
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

	# enable somewhat modern C++
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	
	if("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-comment")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsigned-char")
		if (X86)
			# "x86 vm will crash without -mstackrealign since MMX
			# instructions will be used no matter what and they
			# corrupt the frame pointer in VM calls"
			# -ioquake3 Makefile
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mstackrealign")
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpmath=sse")
		endif()

		if(WIN32)
			# Link libgcc and libstdc++ statically
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++")
		endif()
	elseif("${CMAKE_C_COMPILER_ID}" MATCHES "Clang")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-comment")
	elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "Intel")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -w")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -unroll")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -falign-stack=maintain-16-byte")

		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-intel")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libstdc++")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -static-libgcc")
	endif()

	if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-write-strings")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-comment")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsigned-char")
		if (X86)
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mstackrealign")
			set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mfpmath=sse")
		endif()
	elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-write-strings")
		#set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-writable-strings")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-comment")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-invalid-offsetof")
	elseif("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Intel")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility-inlines-hidden")

		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-intel")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libstdc++")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -static-libgcc")
	endif()
else()
	message(ERROR "Unsupported compiler")
endif()

if (NOT CMAKE_BUILD_TYPE)
	message(STATUS "No build type selected, default to RELEASE")
	set(CMAKE_BUILD_TYPE "RELEASE")
endif()

if(CMAKE_BUILD_TYPE MATCHES "DEBUG" OR CMAKE_BUILD_TYPE MATCHES "Debug")
	# CMake already defines _DEBUG for MSVC.
	if (NOT MSVC)
		set(SharedDefines ${SharedDefines} "_DEBUG")
	endif()
else()
		set(SharedDefines ${SharedDefines} "FINAL_BUILD")
endif()

# https://reproducible-builds.org/specs/source-date-epoch/
if (NOT ("$ENV{SOURCE_DATE_EPOCH}" STREQUAL ""))
	execute_process(COMMAND "date"
		"--date=@$ENV{SOURCE_DATE_EPOCH}" "+%b %_d %Y"
		OUTPUT_VARIABLE source_date
		ERROR_QUIET
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	set(SharedDefines ${SharedDefines} "SOURCE_DATE=\"${source_date}\"")
endif()

# Add projects
add_subdirectory(${JKA_YBEProxyDir})