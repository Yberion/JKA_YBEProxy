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
#include "sdk/qcommon/qcommon.hpp"

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
#define YBEPROXY_VERSION "1.0.0 Beta"
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

// Net ping and timenudge, see Proxy_Engine_NetStatus.cpp
struct netEstimate_t
{
	static constexpr int RttSamples = 64;
	static constexpr int WindowSamples = 128;
	// One second of packets of a client that sends one at every frame (the limit is 1000 frames per second)
	static constexpr int PacketSamples = 1024;

	// Real time the snapshot was sent, and its sequence (a slot is reused after PACKET_BACKUP messages)
	int		sentTime[PACKET_BACKUP];
	int		sentSequence[PACKET_BACKUP];
	int		lastAckedSequence;

	// Real time and number of new usercmds of each packet received
	int		packetTime[PacketSamples];
	int		packetCmds[PacketSamples];
	int		packetIndex;

	int		lastSnapshotTime;
	float	snapshotInterval;

	int		rttTime[RttSamples];
	int		rttValue[RttSamples];
	int		rttIndex;

	// Samples of the current window of one second: the delay of the newest usercmd of each packet, the time each
	// snapshot left after its server time, and the client frames seen between two usercmds in a row
	float	delaySum;
	int		delayCount;
	float	lateSum;
	int		lateCount;
	int		frameLag[WindowSamples];
	int		lagCount;
	int		lastCmdServerTime;
	int		windowStart;
	// Inputs and result of the last simulation of the client, run again only when an input changes
	int		simInterval;
	int		simFrameMs;
	int		simFramesPerPacket;
	float	simulated;

	int		history[3];
	int		historyIndex;
	bool	historyFilled;
};

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
	bool				sendWelcomeMessage;
	// Set by the "mute" console command, cleared when the client leaves
	bool				muted;
	char				cleanName[MAX_NETNAME];
	// IP address of the connection, without the port. Empty until the game accepted the client, and for bots
	char				address[32];

	netEstimate_t		netEstimate;
	int					netPing;
	int					timenudge;
	// False until measured, and when the client gets too few snapshots to tell
	bool				timenudgeKnown;

	int					lastTimeNetStatus;
	int					lastTimeMyratioCheck;
};

struct Proxy_OriginalEngine_CVars_t
{
	// New cvars
	vmCvar_t proxy_sv_pingFix;
	vmCvar_t proxy_sv_enableNetStatus;
	// The engine's cap of the clients' rate, registered so that proxyconfig lists it next to sv_minRate
	vmCvar_t sv_maxRate;
	// Floor of the clients' rate, as in ioq3, Quake3e, OpenJK and JK2MV (see Proxy_Engine_Rate.cpp)
	vmCvar_t sv_minRate;
	// Floor and cap of the clients' snaps, as in JK2MV (see Proxy_Engine_Rate.cpp)
	vmCvar_t sv_minSnaps;
	vmCvar_t sv_maxSnaps;
};

struct Proxy_CVars_t
{
	// New cvars
	vmCvar_t proxy_sv_maxCallVoteMapRestartValue;
	vmCvar_t proxy_sv_modelPathLength;
	vmCvar_t proxy_sv_disableKillCmd;
	vmCvar_t proxy_sv_maxClientsPerIP;
	vmCvar_t proxy_sv_disableWelcomeMessage;
	vmCvar_t proxy_sv_disableServerInfoVersion;

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
