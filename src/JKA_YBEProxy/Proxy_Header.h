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

	#define YbeProxy_OpenLibrary(a) (void *)LoadLibrary(a)
	#define YbeProxy_CloseLibrary(a) FreeLibrary((HMODULE)a)
	#define YbeProxy_GetFunctionAddress(a, b) GetProcAddress((HMODULE)a, b)
#else
	#include <dlfcn.h>

	#define PROXY_LIBRARY_EXT "so"

	#define YbeProxy_OpenLibrary(a, b) dlopen(a, RTLD_NOW)
	#define YbeProxy_CloseLibrary(a) dlclose(a)
	#define YbeProxy_GetFunctionAddress(a, b) dlsym(a, b)
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
#define YBEPROXY_VERSION "0.2.1 Beta"
#define YBEPROXY_BY_AUTHOR "by Yberion"

// ==================================================
// TYPEDEFS
// ==================================================

typedef intptr_t	(QDECL *systemCallFuncPtr_t)(intptr_t command, ...);
typedef intptr_t	(*vmMainFuncPtr_t)(int command, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t, intptr_t);
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
// Proxy_SystemCalls
// ------------------------

void TranslateSystemcalls(void);

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

// VM_DllSyscall can handle up to 1 (command) + 15 args
intptr_t QDECL Proxy_systemCall(intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5,
	intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14);