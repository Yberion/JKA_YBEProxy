// ==================================================
// JKA_YBEProxy by Yberion
// Inspired by JMPProxy from DeathSpike (https://github.com/Deathspike/JMPProxy)
// ==================================================

#pragma once

// ==================================================
// INCLUDES
// ==================================================

#include "game/g_local.h"

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
// DEFINES
// ==================================================

#define FS_GAME_CVAR "fs_game"

#define DEFAULT_BASE_GAME_FOLDER_NAME "base"

#define PROXY_LIBRARY_SLASH "/"
#define PROXY_LIBRARY_NAME "JKA_YBEProxy"
#define PROXY_LIBRARY_DOT "."

#define PROXY_LIBRARY PROXY_LIBRARY_SLASH PROXY_LIBRARY_NAME PROXY_LIBRARY_DOT PROXY_LIBRARY_EXT

#define YBEPROXY_NAME "YbeProxy"
#define YBEPROXY_VERSION "0.3.0 Beta"
#define YBEPROXY_BY_AUTHOR "by Yberion"

// ==================================================
// TYPEDEFS
// ==================================================

typedef intptr_t	(QDECL *systemCallFuncPtr_t)(intptr_t command, ...);
typedef intptr_t	(*vmMainFuncPtr_t)(intptr_t command, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
typedef void		(*dllEntryFuncPtr_t)(void *);

// ==================================================
// STRUCTS
// ==================================================

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

	struct proxyClientData_s {
		qboolean			isConnected;
		char				cleanName[MAX_NETNAME];
	} clientData[MAX_CLIENTS];
} Proxy_t;

// ==================================================
// GLOBALE VARIABLES
// --------------------------------------------------
// This is the only place where variables will be
// defined globally
// ==================================================

extern Proxy_t proxy;
//extern gameImport_t *trap; // in g_local.h

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

// -- server engine
playerState_t* Proxy_GetPlayerStateByClientNum(int num);
void Proxy_ClientCleanName(const char* in, char* out, int outSize);

// --  q_shared
char* QDECL va(const char* format, ...);
char* Info_ValueForKey(const char* s, const char* key);
void Info_RemoveKey(char* s, const char* key);
void Info_SetValueForKey(char* s, const char* key, const char* value);
int QDECL Com_sprintf(char* dest, int size, const char* fmt, ...);

// -- q_string
/*
#if defined (_MSC_VER)
	// vsnprintf is ISO/IEC 9899:1999
	// abstracting this to make it portable
	int Q_vsnprintf(char* str, size_t size, const char* format, va_list ap);
#else // not using MSVC
	#define Q_vsnprintf vsnprintf
#endif
*/

int Q_stricmpn(const char* s1, const char* s2, int n);
int Q_stricmp(const char* s1, const char* s2);
const char* Q_strchrs(const char* string, const char* search);
void Q_strncpyz(char* dest, const char* src, int destsize);

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
void Proxy_NewAPI_ShutdownGame(int restart);
char* Proxy_NewAPI_ClientConnect(int clientNum, qboolean firstTime, qboolean isBot);
void Proxy_NewAPI_ClientBegin(int clientNum, qboolean allowTeamReset);
void Proxy_NewAPI_ClientCommand(int clientNum);
qboolean Proxy_NewAPI_ClientUserinfoChanged(int clientNum);
void Proxy_NewAPI_RunFrame(int levelTime);

// ------------------------
// Proxy_OldAPIWrappers
// ------------------------

// VM_DllSyscall can handle up to 1 (command) + 15 args
intptr_t QDECL Proxy_OldAPI_systemCall(intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5,
	intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14);

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
void Proxy_SharedAPI_ClientUserinfoChanged(int clientNum);

// ------------------------
// Proxy_SystemCalls
// ------------------------

void TranslateSystemCalls(void);