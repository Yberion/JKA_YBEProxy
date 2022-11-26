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

#============================================================================
# src/JKA_YBEProxy
#============================================================================

set(JKA_YBEProxyDefines ${MPSharedDefines} "_GAME" )
set(JKA_YBEProxyMainFiles
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_ClientCommand.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_ClientCommand.hpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_CVars.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_CVars.hpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_Files.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_Files.hpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_Header.hpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_Main.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_Main.hpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_SharedAPI.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_SharedAPI.hpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_Shell.hpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_Translate_SystemCalls.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Proxy_Translate_SystemCalls.hpp"
	)
source_group("src/JKA_YBEProxy" FILES ${JKA_YBEProxyMainFiles})
set(JKA_YBEProxyFiles ${JKA_YBEProxyFiles} ${JKA_YBEProxyMainFiles})

#============================================================================
# src/JKA_YBEProxy/Imports/game
#============================================================================

set(JKA_YBEProxyImportsGameFiles
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Imports/game/g_cmds.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Imports/game/g_cmds.hpp"
	)
source_group("src/JKA_YBEProxy/Imports/game" FILES ${JKA_YBEProxyImportsGameFiles})
set(JKA_YBEProxyFiles ${JKA_YBEProxyFiles} ${JKA_YBEProxyImportsGameFiles})

#============================================================================
# src/JKA_YBEProxy/Imports/qcommon
#============================================================================

set(JKA_YBEProxyImportsQcommonFiles
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Imports/qcommon/cvar.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Imports/qcommon/cvar.hpp"
	)
source_group("src/JKA_YBEProxy/Imports/qcommon" FILES ${JKA_YBEProxyImportsQcommonFiles})
set(JKA_YBEProxyFiles ${JKA_YBEProxyFiles} ${JKA_YBEProxyImportsQcommonFiles})

#============================================================================
# src/JKA_YBEProxy/Imports/server
#============================================================================

set(JKA_YBEProxyImportsServerFiles
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Imports/server/sv_game.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Imports/server/sv_game.hpp"
	)
source_group("src/JKA_YBEProxy/Imports/server" FILES ${JKA_YBEProxyImportsServerFiles})
set(JKA_YBEProxyFiles ${JKA_YBEProxyFiles} ${JKA_YBEProxyImportsServerFiles})

#============================================================================
# src/JKA_YBEProxy/RuntimePatch/Engine
#============================================================================

set(JKA_YBEProxyRuntimePatchEngineFiles
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Client.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Client.hpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_ClientCommand.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_ClientCommand.hpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Patch.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Patch.hpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Utils.hpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Wrappers.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Wrappers.hpp"
	)
source_group("src/JKA_YBEProxy/RuntimePatch/Engine" FILES ${JKA_YBEProxyRuntimePatchEngineFiles})
set(JKA_YBEProxyFiles ${JKA_YBEProxyFiles} ${JKA_YBEProxyRuntimePatchEngineFiles})

#============================================================================
# src/JKA_YBEProxy/RuntimePatch/Engine/Patches/qcommon
#============================================================================

set(JKA_YBEProxyRuntimePatchEnginePatchesQcommonFiles
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Patches/qcommon/Proxy_cmd.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Patches/qcommon/Proxy_cmd.hpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Patches/qcommon/Proxy_common.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Patches/qcommon/Proxy_common.hpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Patches/qcommon/Proxy_files.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Patches/qcommon/Proxy_files.hpp"
	)
source_group("src/JKA_YBEProxy/RuntimePatch/Engine/Patches/qcommon" FILES ${JKA_YBEProxyRuntimePatchEnginePatchesQcommonFiles})
set(JKA_YBEProxyFiles ${JKA_YBEProxyFiles} ${JKA_YBEProxyRuntimePatchEnginePatchesQcommonFiles})

#============================================================================
# src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server
#============================================================================

set(JKA_YBEProxyRuntimePatchEnginePatchesServerFiles
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_ccmds.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_ccmds.hpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_client.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_client.hpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_game.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_game.hpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_main.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_main.hpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_snapshot.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/Engine/Patches/server/Proxy_sv_snapshot.hpp"
	)
source_group("src/JKA_YBEProxy/RuntimePatch/Engine/Patches/server" FILES ${JKA_YBEProxyRuntimePatchEnginePatchesServerFiles})
set(JKA_YBEProxyFiles ${JKA_YBEProxyFiles} ${JKA_YBEProxyRuntimePatchEnginePatchesServerFiles})

#============================================================================
# src/JKA_YBEProxy/RuntimePatch/HookUtils
#============================================================================

set(JKA_YBEProxyHookUtilsFiles
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/HookUtils/HookUtils.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/HookUtils/HookUtils.hpp"
	)
source_group("src/JKA_YBEProxy/RuntimePatch/HookUtils" FILES ${JKA_YBEProxyHookUtilsFiles})
set(JKA_YBEProxyFiles ${JKA_YBEProxyFiles} ${JKA_YBEProxyHookUtilsFiles})

#============================================================================
# src/JKA_YBEProxy/RuntimePatch/HookUtils/LengthDisassembler/
#============================================================================

set(JKA_YBEProxyLengthDisassemblerFiles
	"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/HookUtils/LengthDisassembler/hde.hpp"
	)
source_group("src/JKA_YBEProxy/RuntimePatch/HookUtils/LengthDisassembler" FILES ${JKA_YBEProxyLengthDisassemblerFiles})
set(JKA_YBEProxyFiles ${JKA_YBEProxyFiles} ${JKA_YBEProxyLengthDisassemblerFiles})

if (CMAKE_SIZEOF_VOID_P EQUAL 4 OR "${TARGET_ARCH}" MATCHES "x86")
	#============================================================================
	# src/JKA_YBEProxy/RuntimePatch/HookUtils/LengthDisassembler/x86
	#============================================================================

	set(JKA_YBEProxyLengthDisassemblerX86Files
		"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/HookUtils/LengthDisassembler/x86/hde32.cpp"
		"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/HookUtils/LengthDisassembler/x86/hde32.hpp"
		"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/HookUtils/LengthDisassembler/x86/table32.hpp"
		)
	source_group("src/JKA_YBEProxy/RuntimePatch/HookUtils/LengthDisassembler/x86" FILES ${JKA_YBEProxyLengthDisassemblerX86Files})
	set(JKA_YBEProxyFiles ${JKA_YBEProxyFiles} ${JKA_YBEProxyLengthDisassemblerX86Files})
else ()
	#============================================================================
	# src/JKA_YBEProxy/RuntimePatch/HookUtils/LengthDisassembler/x64
	#============================================================================

	set(JKA_YBEProxyLengthDisassemblerX64Files
		"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/HookUtils/LengthDisassembler/x64/hde64.cpp"
		"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/HookUtils/LengthDisassembler/x64/hde64.hpp"
		"${JKA_YBEProxyDir}/JKA_YBEProxy/RuntimePatch/HookUtils/LengthDisassembler/x64/table64.hpp"
		)
	source_group("src/JKA_YBEProxy/RuntimePatch/HookUtils/LengthDisassembler/x64" FILES ${JKA_YBEProxyLengthDisassemblerX64Files})
	set(JKA_YBEProxyFiles ${JKA_YBEProxyFiles} ${JKA_YBEProxyLengthDisassemblerX64Files})
endif ()

#============================================================================
# src/JKA_YBEProxy/Wrappers
#============================================================================

set(JKA_YBEProxyWrapperFiles
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Wrappers/Proxy_OpenJKAPI_Wrappers.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Wrappers/Proxy_OpenJKAPI_Wrappers.hpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Wrappers/Proxy_OriginalAPI_Wrappers.cpp"
	"${JKA_YBEProxyDir}/JKA_YBEProxy/Wrappers/Proxy_OriginalAPI_Wrappers.hpp"
	)
source_group("src/JKA_YBEProxy/Wrapper" FILES ${JKA_YBEProxyWrapperFiles})
set(JKA_YBEProxyFiles ${JKA_YBEProxyFiles} ${JKA_YBEProxyWrapperFiles})

#============================================================================
# src/sdk/server
#============================================================================

set(JKA_YBEProxyServerFiles
	"${JKA_YBEProxyDir}/sdk/server/server.hpp"
	"${JKA_YBEProxyDir}/sdk/server/sv_client.cpp"
	"${JKA_YBEProxyDir}/sdk/server/sv_client.hpp"
	)
source_group("src/sdk/server" FILES ${JKA_YBEProxyServerFiles})
set(JKA_YBEProxyFiles ${JKA_YBEProxyFiles} ${JKA_YBEProxyServerFiles})

#============================================================================
# src/sdk/sys
#============================================================================

set(JKA_YBEProxySysFiles
	"${JKA_YBEProxyDir}/sdk/sys/sys_public.hpp"
	)
source_group("src/sdk/sys" FILES ${JKA_YBEProxySysFiles})
set(JKA_YBEProxyFiles ${JKA_YBEProxyFiles} ${JKA_YBEProxySysFiles})

#============================================================================
# src/sdk/qcommon
#============================================================================

set(JKA_YBEProxyDSKCommonFiles
	"${JKA_YBEProxyDir}/sdk/qcommon/disablewarnings.hpp"
	"${JKA_YBEProxyDir}/sdk/qcommon/files.cpp"
	"${JKA_YBEProxyDir}/sdk/qcommon/game_version.hpp"
	"${JKA_YBEProxyDir}/sdk/qcommon/q_color.hpp"
	"${JKA_YBEProxyDir}/sdk/qcommon/q_math.hpp"
	"${JKA_YBEProxyDir}/sdk/qcommon/q_platform.hpp"
	"${JKA_YBEProxyDir}/sdk/qcommon/q_shared.cpp"
	"${JKA_YBEProxyDir}/sdk/qcommon/q_shared.hpp"
	"${JKA_YBEProxyDir}/sdk/qcommon/q_string.cpp"
	"${JKA_YBEProxyDir}/sdk/qcommon/q_string.hpp"
	"${JKA_YBEProxyDir}/sdk/qcommon/qcommon.hpp"
	"${JKA_YBEProxyDir}/sdk/qcommon/tags.hpp"
	)
source_group("src/sdk/qcommon" FILES ${JKA_YBEProxyDSKCommonFiles})
set(JKA_YBEProxyFiles ${JKA_YBEProxyFiles} ${JKA_YBEProxyDSKCommonFiles})

#============================================================================
# src/sdk/game
#============================================================================

set(JKA_YBEProxySDKGameFiles
	"${JKA_YBEProxyDir}/sdk/game/ai.hpp"
	"${JKA_YBEProxyDir}/sdk/game/anims.hpp"
	"${JKA_YBEProxyDir}/sdk/game/b_public.hpp"
	"${JKA_YBEProxyDir}/sdk/game/bg_public.hpp"
	"${JKA_YBEProxyDir}/sdk/game/bg_vehicles.hpp"
	"${JKA_YBEProxyDir}/sdk/game/bg_weapons.hpp"
	"${JKA_YBEProxyDir}/sdk/game/g_local.hpp"
	"${JKA_YBEProxyDir}/sdk/game/g_public.hpp"
	"${JKA_YBEProxyDir}/sdk/game/g_team.hpp"
	"${JKA_YBEProxyDir}/sdk/game/g_xcvar.hpp"
	"${JKA_YBEProxyDir}/sdk/game/surfaceflags.hpp"
	"${JKA_YBEProxyDir}/sdk/game/teams.hpp"
	)
source_group("src/sdk/game" FILES ${JKA_YBEProxySDKGameFiles})
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
