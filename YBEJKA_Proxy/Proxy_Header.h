// ==================================================
// YBEJKA_Proxy by Yberion
// Inspired by JMPProxy from DeathSpike (https://github.com/Deathspike/JMPProxy)
// ==================================================

#pragma once

// ==================================================
// INCLUDES
// ==================================================

#include "JKA_SDK/game/g_local.h"

// ==================================================
// PLATFORM SPECIFIC STUFF
// ==================================================

#ifdef WIN32
	#include <windows.h>

	#define PROXY_LIBRARY_EXT "dll"

	#define Q_EXPORT __declspec(dllexport)

	#define YbeProxy_OpenLibrary(a) LoadLibrary(a)
	#define YbeProxy_CloseLibrary(a) FreeLibrary(a)
	#define YbeProxy_GetFunctionAddress(a, b) GetProcAddress(a, b)
#else
	#define PROXY_LIBRARY_EXT "so"

	#define Q_EXPORT

	#define YbeProxy_OpenLibrary(a, b) dlopen(a, b)
	#define YbeProxy_CloseLibrary(a) dlclose(a)
	#define YbeProxy_GetFunctionAddress(a, b) dlsym(a, b)
#endif

// ==================================================
// DEFINES
// ==================================================

#define FS_GAME_CVAR "fs_game"

#define DEFAULT_BASE_GAME_FOLDER_NAME "base"

#define PROXY_LIBRARY_SLASH "/"
#define PROXY_LIBRARY_NAME "YBEJKA_Proxy"
#define PROXY_LIBRARY_DOT "."

#define PROXY_LIBRARY PROXY_LIBRARY_SLASH PROXY_LIBRARY_NAME PROXY_LIBRARY_DOT PROXY_LIBRARY_EXT

#define YBEPROXY_NAME "YbeProxy"
#define YBEPROXY_VERSION "0.1.0"
#define YBEPROXY_BY_AUTHOR "by Yberion"

// ==================================================
// TYPEDEFS
// ==================================================

typedef int		(QDECL *systemCallFuncPtr_t)(int command, ...);
typedef int		(*vmMainFuncPtr_t)(int command, int, int, int, int, int, int, int, int, int, int, int, int);
typedef void	(*dllEntryFuncPtr_t)(systemCallFuncPtr_t);

// ==================================================
// STRUCTS
// ==================================================

typedef struct Proxy_s {
	void					*jampgameHandle;

	vmMainFuncPtr_t			originalVmMain;
	dllEntryFuncPtr_t		originalDllEntry;
	systemCallFuncPtr_t		originalSystemCall;

	int						originalVmMainResponse;
} Proxy_t;

// ==================================================
// GLOBALE VARIABLES
// --------------------------------------------------
// This is the only place where variables will be
// defined globally
// ==================================================

extern Proxy_t proxy;

// ==================================================
// FUNCTIONS
// ==================================================

// ------------------------
// Proxy_Imports 
// ------------------------

char	*QDECL va(const char* format, ...);

#if defined (_MSC_VER)
	// vsnprintf is ISO/IEC 9899:1999
	// abstracting this to make it portable
	int Q_vsnprintf(char* str, size_t size, const char* format, va_list args);
#else // not using MSVC
	#define Q_vsnprintf vsnprintf
#endif

// ------------------------
// Proxy_Wrappers
// ------------------------

int QDECL Proxy_systemCall(int command, int* arg1, int* arg2, int* arg3, int* arg4, int* arg5, int* arg6, int* arg7, int* arg8, int* arg9, int* arg10);