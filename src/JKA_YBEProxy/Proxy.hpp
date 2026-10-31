// ==================================================
// JKA_YBEProxy by Yberion
// Inspired by JMPProxy from DeathSpike (https://github.com/Deathspike/JMPProxy)
// ==================================================

#pragma once

// ==================================================
// INCLUDE
// ==================================================

#include "JKA_YBEProxy/Platform.hpp"
#include "sdk/game/g_local.hpp"
#include "sdk/game/g_public.hpp"
#include "sdk/qcommon/q_shared.hpp"
#include "sdk/qcommon/qcommon.hpp"

#include <cstdint>
#include <cstddef>

// ==================================================
// DEFINE
// ==================================================

#define YBEPROXY_NAME "YbeProxy"
#define YBEPROXY_VERSION "1.0.0 Beta"
#define YBEPROXY_BY_AUTHOR "by Yberion"

// ==================================================
// TYPEDEF
// ==================================================

using systemCallFuncPtr_t =	intptr_t (QDECL *)(intptr_t command, ...);
using vmMainFuncPtr_t = 	intptr_t (*)(intptr_t command, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
using dllEntryFuncPtr_t = 	void (*)(systemCallFuncPtr_t);

namespace ybeproxy
{

// ==================================================
// STRUCTS
// ==================================================

// Net ping and timenudge, see Engine/NetStatus.cpp
struct NetEstimate
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

struct LocatedGameData
{
	sharedEntity_t*		g_entities;
	int					g_entitySize;
	int					num_entities;

	playerState_t*		g_clients;
	int					g_clientSize;
};

// /netStatus of a client, original engine only (see Engine/NetStatus.cpp)
struct ClientNetStatus
{
	NetEstimate		estimate;
	int					netPing;
	int					timenudge;
	// False until measured, and when the client gets too few snapshots to tell
	bool				timenudgeKnown;
	// Server time of the last /netStatus or /explainNetStatus of the client
	int					lastCommandTime;
};

struct ClientData
{
	bool				isConnected;
	bool				sendWelcomeMessage;
	// Set by the "mute" console command, cleared when the client leaves
	bool				muted;
	char				cleanName[MAX_NETNAME];
	// IP address of the connection, without the port. Empty until the game accepted the client, and for bots
	char				address[32];

	ClientNetStatus	netStatus;
	// levelTime of the last /myratio of the client
	int					lastTimeMyratioCheck;
};

struct OriginalEngineCvars
{
	// New cvars
	vmCvar_t proxy_sv_pingFix;
	vmCvar_t proxy_sv_enableNetStatus;
	// The engine's cap of the clients' rate, registered so that proxyconfig lists it next to sv_minRate
	vmCvar_t sv_maxRate;
	// Floor of the clients' rate, as in ioq3, Quake3e, OpenJK and JK2MV (see Engine/Rate.cpp)
	vmCvar_t sv_minRate;
	// Floor and cap of the clients' snaps, as in JK2MV (see Engine/Rate.cpp)
	vmCvar_t sv_minSnaps;
	vmCvar_t sv_maxSnaps;
};

struct ProxyCvars
{
	// New cvars
	vmCvar_t proxy_sv_maxCallVoteMapRestartValue;
	vmCvar_t proxy_sv_modelPathLength;
	vmCvar_t proxy_sv_disableKillCmd;
	vmCvar_t proxy_sv_maxClientsPerIP;
	vmCvar_t proxy_sv_disableWelcomeMessage;
	vmCvar_t proxy_sv_disableServerInfoVersion;
	vmCvar_t proxy_sv_allowSamePlayerNames;

	// get cvars
	vmCvar_t sv_fps;
	vmCvar_t sv_gametype;
};

// The original game module (see Module/GameLibrary.cpp)
struct OriginalGame
{
	void*					handle;
	vmMainFuncPtr_t			vmMain;
	dllEntryFuncPtr_t		dllEntry;
};

struct Proxy
{
	OriginalGame			originalGame;

	// The system call of the engine, and the functions of the proxy built on it (see Module/EngineApi.cpp)
	systemCallFuncPtr_t		originalSystemCall;
	gameImport_t*			trap;

	bool					isOriginalEngine;

	// levelTime of the last GAME_RUN_FRAME
	int						levelTime;
	LocatedGameData		locatedGameData;
	ClientData			clientData[MAX_CLIENTS];
	
	OriginalEngineCvars originalEngineCvars;
	ProxyCvars			 cvars;
};

// ==================================================
// EXTERN VARIABLE
// ==================================================

extern Proxy proxy;

} // namespace ybeproxy
