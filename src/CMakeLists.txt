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
if(NOT InJKA_YBEProxy)
	message(FATAL_ERROR "Use the top-level cmake script!")
endif(NOT InJKA_YBEProxy)

set(MPSharedDefines ${SharedDefines})

set(JKA_YBEProxyIncludeDirectories "${JKA_YBEProxyDir}")

if(WIN32)
	set(JKA_YBEProxyLibraries "winmm")
endif(WIN32)

set(JKA_YBEProxyDefines ${MPSharedDefines} "_GAME" )
set(JKA_YBEProxyMainFiles
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_ClientCommand.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_Files.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_Imports.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_Main.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_NewAPIWrappers.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_OldAPIWrappers.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_SystemCalls.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_Patch.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_Patch.hpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_Server.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_Server.hpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_SharedAPI.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_Shell.hpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_Header.hpp"
	)
source_group("JKA_YBEProxy" FILES ${JKA_YBEProxyMainFiles})
set(JKA_YBEProxyFiles ${JKA_YBEProxyFiles} ${JKA_YBEProxyMainFiles})

set(JKA_YBEProxyDetourFiles
	"${JKA_YBEProxyDir}/JKA_YBEProxy/DetourPatcher/DetourPatcher.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/DetourPatcher/DetourPatcher.hpp"
	)
source_group("DetourPatcher" FILES ${JKA_YBEProxyDetourFiles})
set(JKA_YBEProxyFiles ${JKA_YBEProxyFiles} ${JKA_YBEProxyDetourFiles})

set(JKA_YBEProxyEnginePatchFiles
	"${JKA_YBEProxyDir}/JKA_YBEProxy/EnginePatch/Proxy_common.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/EnginePatch/Proxy_EnginePatch.hpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/EnginePatch/Proxy_sv_client.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/EnginePatch/Proxy_sv_game.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/EnginePatch/Proxy_sv_main.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/EnginePatch/Proxy_sv_snapshot.cpp"
	)
source_group("EnginePatch" FILES ${JKA_YBEProxyEnginePatchFiles})
set(JKA_YBEProxyFiles ${JKA_YBEProxyFiles} ${JKA_YBEProxyEnginePatchFiles})

set(JKA_YBEProxyServerFiles
	"${JKA_YBEProxyDir}/server/server.hpp"
	)
source_group("server" FILES ${JKA_YBEProxyServerFiles})
set(JKA_YBEProxyFiles ${JKA_YBEProxyFiles} ${JKA_YBEProxyServerFiles})

set(JKA_YBEProxySysFiles
	"${JKA_YBEProxyDir}/sys/sys_public.hpp"
	)
source_group("sys" FILES ${JKA_YBEProxySysFiles})
set(JKA_YBEProxyFiles ${JKA_YBEProxyFiles} ${JKA_YBEProxySysFiles})

set(JKA_YBEProxyDSKCommonFiles
	"${JKA_YBEProxyDir}/qcommon/disablewarnings.hpp"
	"${JKA_YBEProxyDir}/qcommon/q_color.hpp"
	"${JKA_YBEProxyDir}/qcommon/q_math.hpp"
	"${JKA_YBEProxyDir}/qcommon/q_platform.hpp"
	"${JKA_YBEProxyDir}/qcommon/q_shared.hpp"
	"${JKA_YBEProxyDir}/qcommon/q_string.hpp"
	"${JKA_YBEProxyDir}/qcommon/qcommon.hpp"
	"${JKA_YBEProxyDir}/qcommon/tags.hpp"
	)
source_group("qcommon" FILES ${JKA_YBEProxyDSKCommonFiles})
set(JKA_YBEProxyFiles ${JKA_YBEProxyFiles} ${JKA_YBEProxyDSKCommonFiles})

set(JKA_YBEProxySDKGameFiles
	"${JKA_YBEProxyDir}/game/ai.hpp"
	"${JKA_YBEProxyDir}/game/anims.hpp"
	"${JKA_YBEProxyDir}/game/b_public.hpp"
	"${JKA_YBEProxyDir}/game/bg_public.hpp"
	"${JKA_YBEProxyDir}/game/bg_vehicles.hpp"
	"${JKA_YBEProxyDir}/game/bg_weapons.hpp"
	"${JKA_YBEProxyDir}/game/g_local.hpp"
	"${JKA_YBEProxyDir}/game/g_public.hpp"
	"${JKA_YBEProxyDir}/game/g_team.hpp"
	"${JKA_YBEProxyDir}/game/g_xcvar.hpp"
	"${JKA_YBEProxyDir}/game/surfaceflags.hpp"
	"${JKA_YBEProxyDir}/game/teams.hpp"
	)
source_group("game" FILES ${JKA_YBEProxySDKGameFiles})
set(JKA_YBEProxyFiles ${JKA_YBEProxyFiles} ${JKA_YBEProxySDKGameFiles})

add_library(${JKA_YBEProxy} SHARED ${JKA_YBEProxyFiles})

if(NOT MSVC)
	# remove "lib" prefix for .so/.dylib files
	set_target_properties(${JKA_YBEProxy} PROPERTIES PREFIX "")
endif()
set_target_properties(${JKA_YBEProxy} PROPERTIES COMPILE_DEFINITIONS "${JKA_YBEProxyDefines}")

# Hide symbols not explicitly marked public.
set_property(TARGET ${JKA_YBEProxy} APPEND PROPERTY COMPILE_OPTIONS ${JKA_YBEProxy_VISIBILITY_FLAGS})

set_target_properties(${JKA_YBEProxy} PROPERTIES INCLUDE_DIRECTORIES "${JKA_YBEProxyIncludeDirectories}")
set_target_properties(${JKA_YBEProxy} PROPERTIES PROJECT_LABEL "JKA_YBEProxy Library")
# no libraries used
if(JKA_YBEProxyLibraries)
	target_link_libraries(${JKA_YBEProxy} ${JKA_YBEProxyLibraries})
endif(JKA_YBEProxyLibraries)

set(JKA_YBEProxyLibsBuilt)
if(BuildJKA_YBEProxy)
	set(JKA_YBEProxyLibsBuilt ${JKA_YBEProxyLibsBuilt} ${JKA_YBEProxy})
endif()

if(WIN32)
	set(JKA_YBEProxyLibFullPaths)
	if(MSVC)
		foreach(JKA_YBEProxyLib ${JKA_YBEProxyLibsBuilt})
			set(JKA_YBEProxyLibFullPaths
				${JKA_YBEProxyLibFullPaths}
				${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${JKA_YBEProxyLib}${CMAKE_SHARED_LIBRARY_SUFFIX})
		endforeach(JKA_YBEProxyLib)
	else()
		foreach(JKA_YBEProxyLib ${JKA_YBEProxyLibsBuilt})
			set(JKA_YBEProxyLibFullPaths
				${JKA_YBEProxyLibFullPaths}
				${CMAKE_BINARY_DIR}/${JKA_YBEProxyLib}${CMAKE_SHARED_LIBRARY_SUFFIX})
		endforeach(JKA_YBEProxyLib)
	endif()
endif()
