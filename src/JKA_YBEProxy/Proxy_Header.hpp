// ==================================================
// JKA_YBEProxy by Yberion
// Inspired by JMPProxy from DeathSpike (https://github.com/Deathspike/JMPProxy)
// ==================================================

#pragma once

// ==================================================
// PLATFORM SPECIFIC STUFF
// ==================================================

// /!\ Should start at 0 because it will be used as index to array
enum Proxy_Platform_e
{
	PROXY_WINDOWS = 0,
	PROXY_LINUX,

	PROXY_PLATFORM_LENGTH
};

#ifdef _MSC_VER
	#include <windows.h>

	#define PROXY_LIBRARY_EXT "dll"

	#define YBEProxy_OpenLibrary(a) (void *)LoadLibrary(a)
	#define YBEProxy_CloseLibrary(a) FreeLibrary((HMODULE)a)
	#define YBEProxy_GetFunctionAddress(a, b) GetProcAddress((HMODULE)a, b)

	#define ORIGINAL_ENGINE_VERSION "(internal)JAmp: v1.0.1.0 win-x86 Oct 30 2003"

	constexpr Proxy_Platform_e PROXY_PLATFORM = Proxy_Platform_e::PROXY_WINDOWS;
#else
	#include <dlfcn.h>

	#define PROXY_LIBRARY_EXT "so"

	#define YBEProxy_OpenLibrary(a) dlopen(a, RTLD_NOW)
	#define YBEProxy_CloseLibrary(a) dlclose(a)
	#define YBEProxy_GetFunctionAddress(a, b) dlsym(a, b)

	#define ORIGINAL_ENGINE_VERSION "JAmp: v1.0.1.1 linux-i386 Nov 10 2003"

	constexpr Proxy_Platform_e PROXY_PLATFORM = Proxy_Platform_e::PROXY_LINUX;
#endif

// ==================================================
// INCLUDE
// ==================================================

#include "sdk/game/g_local.hpp"
#include "sdk/game/g_public.hpp"
#include "sdk/qcommon/q_shared.hpp"

#include <cstdint>
#include <cstddef>

// ==================================================
// DEFINE
// ==================================================

#define FS_GAME_CVAR "fs_game"

#define DEFAULT_BASE_GAME_FOLDER_NAME "base"

#define PROXY_LIBRARY_SLASH "/"
#define PROXY_LIBRARY_NAME "JKA_YBEProxy"
#define PROXY_LIBRARY_DOT "."

#define PROXY_LIBRARY PROXY_LIBRARY_SLASH PROXY_LIBRARY_NAME PROXY_LIBRARY_DOT PROXY_LIBRARY_EXT

#define YBEPROXY_NAME "YbeProxy"
#define YBEPROXY_VERSION "0.18.0 Beta"
#define YBEPROXY_BY_AUTHOR "by Yberion"

// ==================================================
// TYPEDEF
// ==================================================

using systemCallFuncPtr_t =	intptr_t (QDECL *)(intptr_t command, ...);
using vmMainFuncPtr_t = 	intptr_t (*)(intptr_t command, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
using dllEntryFuncPtr_t = 	void (*)(systemCallFuncPtr_t);

// ==================================================
// STRUCTS
// ==================================================

struct timenudgeData_t
{
	int             delayCount;
	int             delaySum;
	int             pingSum;
	int             lastTimeTimeNudgeCalculation;
};

struct ucmdStat_t
{
	int				serverTime;
	std::size_t		packetIndex;
};

#define CMD_MASK 1024

struct ProxyData_t
{
	int					svsTime;
};

struct LocatedGameData_t
{
	sharedEntity_t*		g_entities;
	int					g_entitySize;
	int					num_entities;

	playerState_t*		g_clients;
	int					g_clientSize;
};

struct ClientData_t
{
	bool				isConnected;
	char				cleanName[MAX_NETNAME];

	timenudgeData_t		timenudgeData;
	int					timenudge; // Approximation (+- 7 with stable connection)

	int					lastTimeNetStatus;
	int					lastTimeMyratioCheck;

	ucmdStat_t			cmdStats[CMD_MASK];
	std::size_t			cmdIndex;
};

struct Proxy_OriginalEngine_CVars_t
{
	// New cvars
	vmCvar_t proxy_sv_pingFix;
	vmCvar_t proxy_sv_enableRconCmdCooldown;
	vmCvar_t proxy_sv_enableNetStatus;
};

struct Proxy_CVars_t
{
	// New cvars
	vmCvar_t proxy_sv_maxCallVoteMapRestartValue;
	vmCvar_t proxy_sv_modelPathLength;

	// get cvars
	vmCvar_t sv_fps;
	vmCvar_t sv_gametype;
};

struct Proxy_t
{
	void					*jampgameHandle;

	vmMainFuncPtr_t			originalVmMain;
	dllEntryFuncPtr_t		originalDllEntry;
	systemCallFuncPtr_t		originalSystemCall;

	intptr_t				originalVmMainResponse;

	gameImport_t*			trap;

	gameImport_t*			originalOpenJKAPIGameImportTable;
	gameExport_t*			originalOpenJKAPIGameExportTable;

	gameImport_t*			copyOpenJKAPIGameImportTable;
	gameExport_t*			copyOpenJKAPIGameExportTable;

	bool					isOriginalEngine;

	ProxyData_t				proxyData;
	LocatedGameData_t		locatedGameData;
	ClientData_t			clientData[MAX_CLIENTS];
	
	Proxy_OriginalEngine_CVars_t originalEngineCvars;
	Proxy_CVars_t			 cvars;
};

// ==================================================
// EXTERN VARIABLE
// ==================================================

extern Proxy_t proxy;
