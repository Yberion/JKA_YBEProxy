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

# Make sure the user is not executing this script directly
if(NOT InYBEProxy_JKA)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InYBEProxy_JKA)

set(MPSharedDefines ${SharedDefines})

set(YBEProxy_JKAIncludeDirectories "${YBEProxy_JKADir}")

if(WIN32)
	set(YBEProxy_JKALibraries "winmm")
endif(WIN32)

set(YBEProxy_JKADefines ${MPSharedDefines} "_GAME" )
set(YBEProxy_JKAMainFiles
	"${YBEProxy_JKADir}/Proxy_Imports.c"
	"${YBEProxy_JKADir}/Proxy_Main.c"
	"${YBEProxy_JKADir}/Proxy_SystemCalls.c"
	"${YBEProxy_JKADir}/Proxy_Wrappers.c"
	"${YBEProxy_JKADir}/Proxy_Header.h"
	)
source_group("" FILES ${YBEProxy_JKAMainFiles})
set(YBEProxy_JKAFiles ${YBEProxy_JKAFiles} ${YBEProxy_JKAMainFiles})

set(YBEProxy_JKADSKCommonFiles
	"${YBEProxy_JKADir}/JKA_SDK/qcommon/disablewarnings.h"
	"${YBEProxy_JKADir}/JKA_SDK/qcommon/tags.h"
	)
source_group("JKA_SDK/qcommon" FILES ${YBEProxy_JKADSKCommonFiles})
set(YBEProxy_JKAFiles ${YBEProxy_JKAFiles} ${YBEProxy_JKADSKCommonFiles})

set(YBEProxy_JKADSKFiles
	"${YBEProxy_JKADir}/JKA_SDK/namespace_begin.h"
	"${YBEProxy_JKADir}/JKA_SDK/namespace_end.h"
	)
source_group("JKA_SDK" FILES ${YBEProxy_JKADSKFiles})
set(YBEProxy_JKAFiles ${YBEProxy_JKAFiles} ${YBEProxy_JKADSKFiles})

set(YBEProxy_JKASDKGameFiles
	"${YBEProxy_JKADir}/JKA_SDK/game/ai.h"
	"${YBEProxy_JKADir}/JKA_SDK/game/anims.h"
	"${YBEProxy_JKADir}/JKA_SDK/game/b_public.h"
	"${YBEProxy_JKADir}/JKA_SDK/game/bg_public.h"
	"${YBEProxy_JKADir}/JKA_SDK/game/bg_vehicles.h"
	"${YBEProxy_JKADir}/JKA_SDK/game/bg_weapons.h"
	"${YBEProxy_JKADir}/JKA_SDK/game/g_local.h"
	"${YBEProxy_JKADir}/JKA_SDK/game/g_public.h"
	"${YBEProxy_JKADir}/JKA_SDK/game/g_team.h"
	"${YBEProxy_JKADir}/JKA_SDK/game/q_shared.h"
	"${YBEProxy_JKADir}/JKA_SDK/game/surfaceflags.h"
	"${YBEProxy_JKADir}/JKA_SDK/game/teams.h"
	)
source_group("JKA_SDK/game" FILES ${YBEProxy_JKASDKGameFiles})
set(YBEProxy_JKAFiles ${YBEProxy_JKAFiles} ${YBEProxy_JKASDKGameFiles})

add_library(${YBEProxy_JKA} SHARED ${YBEProxy_JKAFiles})

if(NOT MSVC)
	# remove "lib" prefix for .so/.dylib files
	set_target_properties(${YBEProxy_JKA} PROPERTIES PREFIX "")
endif()
set_target_properties(${YBEProxy_JKA} PROPERTIES COMPILE_DEFINITIONS "${YBEProxy_JKADefines}")

# Hide symbols not explicitly marked public.
set_property(TARGET ${YBEProxy_JKA} APPEND PROPERTY COMPILE_OPTIONS ${YBEProxy_JKA_VISIBILITY_FLAGS})

set_target_properties(${YBEProxy_JKA} PROPERTIES INCLUDE_DIRECTORIES "${YBEProxy_JKAIncludeDirectories}")
set_target_properties(${YBEProxy_JKA} PROPERTIES PROJECT_LABEL "YBEProxy_JKA Library")
# no libraries used
if(YBEProxy_JKALibraries)
	target_link_libraries(${YBEProxy_JKA} ${YBEProxy_JKALibraries})
endif(YBEProxy_JKALibraries)

set(YBEProxy_JKALibsBuilt)
if(BuildYBEProxy_JKA)
	set(YBEProxy_JKALibsBuilt ${YBEProxy_JKALibsBuilt} ${YBEProxy_JKA})
endif()

if(WIN32)
	set(YBEProxy_JKALibFullPaths)
	if(MSVC)
		foreach(YBEProxy_JKALib ${YBEProxy_JKALibsBuilt})
			set(YBEProxy_JKALibFullPaths
				${YBEProxy_JKALibFullPaths}
				${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${YBEProxy_JKALib}${CMAKE_SHARED_LIBRARY_SUFFIX})
		endforeach(YBEProxy_JKALib)
	else()
		foreach(YBEProxy_JKALib ${YBEProxy_JKALibsBuilt})
			set(YBEProxy_JKALibFullPaths
				${YBEProxy_JKALibFullPaths}
				${CMAKE_BINARY_DIR}/${YBEProxy_JKALib}${CMAKE_SHARED_LIBRARY_SUFFIX})
		endforeach(YBEProxy_JKALib)
	endif()
endif()