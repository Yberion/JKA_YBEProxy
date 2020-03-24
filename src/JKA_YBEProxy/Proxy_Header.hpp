// ==================================================
// JKA_YBEProxy by Yberion
// Inspired by JMPProxy from DeathSpike (https://github.com/Deathspike/JMPProxy)
// ==================================================

#pragma once

// ==================================================
// PLATFORM SPECIFIC STUFF
// ==================================================

#ifdef _MSC_VER
	#include <windows.h>

	#define PROXY_LIBRARY_EXT "dll"

	#define YBEProxy_OpenLibrary(a) (void *)LoadLibrary(a)
	#define YBEProxy_CloseLibrary(a) FreeLibrary((HMODULE)a)
	#define YBEProxy_GetFunctionAddress(a, b) GetProcAddress((HMODULE)a, b)
#else
	#include <dlfcn.h>

	#define PROXY_LIBRARY_EXT "so"

	#define YBEProxy_OpenLibrary(a) dlopen(a, RTLD_NOW)
	#define YBEProxy_CloseLibrary(a) dlclose(a)
	#define YBEProxy_GetFunctionAddress(a, b) dlsym(a, b)
#endif

// ==================================================
// INCLUDES
// ==================================================

#include "game/g_local.hpp"
#include "server/server.hpp"
#include "Proxy_Server.hpp"

// ==================================================
// DEFINES
// ==================================================

#define FS_GAME_CVAR "fs_game"

#define DEFAULT_BASE_GAME_FOLDER_NAME "base"

#define PROXY_LIBRARY_SLASH "/"
#define PROXY_LIBRARY_NAME "JKA_YBEProxy"
#define PROXY_LIBRARY_DOT "."

#define PROXY_LIBRARY PROXY_LIBRARY_SLASH PROXY_LIBRARY_NAME PROXY_LIBRARY_DOT PROXY_LIBRARY_EXT

#define YBEPROXY_NAME "YbeProxy"
#define YBEPROXY_VERSION "0.5.2 Beta"
#define YBEPROXY_BY_AUTHOR "by Yberion"

// ==================================================
// TYPEDEFS
// ==================================================

typedef intptr_t	(QDECL *systemCallFuncPtr_t)(intptr_t command, ...);
typedef intptr_t	(*vmMainFuncPtr_t)(intptr_t command, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
typedef void		(*dllEntryFuncPtr_t)(systemCallFuncPtr_t);

// ==================================================
// STRUCTS
// ==================================================

typedef struct timenudgeData_s
{
	int             delayCount;
	int             delaySum;
	int             pingSum;
	int             lastTimeTimeNudgeCalculation;
} timenudgeData_t;

typedef struct ucmdStat_s
{
	int		serverTime;
	int		packetIndex;
} ucmdStat_t;

#define CMD_MASK 1024

typedef struct Proxy_s {
	void					*jampgameHandle;

	vmMainFuncPtr_t			originalVmMain;
	dllEntryFuncPtr_t		originalDllEntry;
	systemCallFuncPtr_t		originalSystemCall;

	intptr_t				originalVmMainResponse;

	gameImport_t*			trap;

	gameImport_t*			originalNewAPIGameImportTable;
	gameExport_t*			originalNewAPIGameExportTable;

	gameImport_t*			copyNewAPIGameImportTable;
	gameExport_t*			copyNewAPIGameExportTable;

	struct locatedGameData_s {
		sharedEntity_t*		g_entities;
		int					g_entitySize;
		int					num_entities;

		playerState_t*		g_clients;
		int					g_clientSize;
	} locatedGameData;

	struct clientData_s {
		qboolean			isConnected;
		char				cleanName[MAX_NETNAME];

		timenudgeData_t		timenudgeData;
		int					timenudge; // Approximation (+- 7 with stable connection)

		int					lastTimeNetStatus;
		int					lastTimeMyratioCheck;

		ucmdStat_t			cmdStats[CMD_MASK];
		int					cmdIndex;
	} clientData[MAX_CLIENTS];

	struct ProxyServer_s
	{
		serverStatic_t*		svs;
		server_t*			sv;
		serverFunctions_t	functions;
		serverCvars_t		cvars;
		common_t			common;
	} server;
} Proxy_t;

// ==================================================
// EXTERN VARIABLE
// ==================================================

extern Proxy_t proxy;

// ==================================================
// FUNCTIONS
// ==================================================

// ------------------------
// Proxy_Files
// ------------------------

void Proxy_LoadOriginalGameLibrary(void);

// ------------------------
// Proxy_Imports
// ------------------------

// -- server utilities
playerState_t* Proxy_GetPlayerStateByClientNum(int num);
void Proxy_ClientCleanName(const char* in, char* out, int outSize);

// -- other
char* ConcatArgs(int start);

// ------------------------
// Proxy_NewAPIWrappers
// ------------------------

void Proxy_NewAPI_InitLayerExportTable(void);
void Proxy_NewAPI_InitLayerImportTable(void);

// -- Import table
void Proxy_NewAPI_LocateGameData(sharedEntity_t* gEnts, int numGEntities, int sizeofGEntity_t, playerState_t* clients, int sizeofGameClient);
void Proxy_NewAPI_GetUsercmd(int clientNum, usercmd_t* cmd);

// -- Export table
void Proxy_NewAPI_ClientBegin(int clientNum, qboolean allowTeamReset);
void Proxy_NewAPI_ClientCommand(int clientNum);
char* Proxy_NewAPI_ClientConnect(int clientNum, qboolean firstTime, qboolean isBot);
void Proxy_NewAPI_ClientThink(int clientNum, usercmd_t* ucmd);
qboolean Proxy_NewAPI_ClientUserinfoChanged(int clientNum);
void Proxy_NewAPI_RunFrame(int levelTime);
void Proxy_NewAPI_ShutdownGame(int restart);

// ------------------------
// Proxy_OldAPIWrappers
// ------------------------

// VM_DllSyscall can handle up to 1 (command) + 15 args
intptr_t QDECL Proxy_OldAPI_systemCall(intptr_t command, ...);

// ------------------------
// Proxy_SharedAPI
// ------------------------

// -- Import table
void Proxy_SharedAPI_LocateGameData(sharedEntity_t* gEnts, int numGEntities, int sizeofGEntity_t, playerState_t* clients, int sizeofGameClient);
void Proxy_SharedAPI_GetUsercmd(int clientNum, usercmd_t* cmd);

// -- Export table
void Proxy_SharedAPI_ClientConnect(int clientNum, qboolean firstTime, qboolean isBot);
void Proxy_SharedAPI_ClientBegin(int clientNum, qboolean allowTeamReset);
qboolean Proxy_SharedAPI_ClientCommand(int clientNum);
void Proxy_SharedAPI_ClientThink(int clientNum, usercmd_t* ucmd);
void Proxy_SharedAPI_ClientUserinfoChanged(int clientNum);

// ------------------------
// Proxy_SystemCalls
// ------------------------

void TranslateSystemCalls(void);

// ------------------------
// Proxy_Patch
// ------------------------

void Proxy_Patch_Attach(void);
void Proxy_Patch_Detach(void);

// ------------------------
// Proxy_Server
// ------------------------

void Proxy_Server_Initialize_MemoryAddress(void);
void Proxy_Server_CalcPacketsAndFPS(int clientNum, int* packets, int* fps);
void Proxy_Server_UpdateUcmdStats(int clientNum, usercmd_t* cmd, int packetIndex);
void Proxy_Server_UpdateTimenudge(client_t* client, usercmd_t* cmd, int _Milliseconds);

// ------------------------
// Proxy_ClientCommand
// ------------------------

void Proxy_ClientCommand_NetStatus(int clientNum);
void Proxy_ClientCommand_MyRatio(int clientNum);
