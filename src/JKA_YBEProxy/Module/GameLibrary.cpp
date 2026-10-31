#include "JKA_YBEProxy/Module/GameLibrary.hpp"
#include "JKA_YBEProxy/Proxy.hpp"
#include "JKA_YBEProxy/Module/GameImports.hpp"

#define FS_GAME_CVAR "fs_game"

#define DEFAULT_BASE_GAME_FOLDER_NAME "base"

#define PROXY_LIBRARY_SLASH "/"
#define PROXY_LIBRARY_NAME "JKA_YBEProxy"
#define PROXY_LIBRARY_DOT "."

#define PROXY_LIBRARY PROXY_LIBRARY_SLASH PROXY_LIBRARY_NAME PROXY_LIBRARY_DOT PROXY_LIBRARY_EXT

static void Proxy_LoadGameLibrary(void)
{
	char basePath[MAX_OSPATH] = { 0 };
	char gameDir[MAX_OSPATH] = { 0 };
	char libraryPath[MAX_OSPATH];
	int libraryPathLength = 0;

	proxy.trap->Cvar_VariableStringBuffer("fs_basepath", basePath, sizeof(basePath));
	proxy.trap->Cvar_VariableStringBuffer(FS_GAME_CVAR, gameDir, sizeof(gameDir));

	// empty means that fs_game is set to the default value which is the default base game folder name
	if (!gameDir[0])
	{
		Q_strncpyz(gameDir, DEFAULT_BASE_GAME_FOLDER_NAME, sizeof(gameDir));
	}

	// From fs_basepath, as the engine does for the game module on Linux. fs_basepath defaults to the
	// current directory, so with the default value this is the same file as before
	if (basePath[0])
	{
		libraryPathLength = Com_sprintf(libraryPath, sizeof(libraryPath), "%s/%s%s", basePath, gameDir, PROXY_LIBRARY);
	}
	else
	{
		libraryPathLength = Com_sprintf(libraryPath, sizeof(libraryPath), "%s%s", gameDir, PROXY_LIBRARY);
	}

	// A truncated path would open another file
	if (libraryPathLength < 0 || libraryPathLength >= (int)sizeof(libraryPath))
	{
		proxy.trap->Print("================================================================\n");
		proxy.trap->Print("----- Proxy: The path of %s is too long, exiting\n", PROXY_LIBRARY_NAME PROXY_LIBRARY_DOT PROXY_LIBRARY_EXT);
		proxy.trap->Print("================================================================\n");

		exit(EXIT_FAILURE);
	}

	proxy.originalGame.handle = YBEProxy_OpenLibrary(libraryPath);

	// exit() as the engine does when it can't load a game module (ERR_FATAL): no map can run without it
	if (proxy.originalGame.handle == NULL)
	{
		proxy.trap->Print("================================================================\n");
		proxy.trap->Print("----- Proxy: Failed to open %s, exiting\n", libraryPath);
		proxy.trap->Print("================================================================\n");

		exit(EXIT_FAILURE);
	}
}

static void Proxy_GetOriginalGameAPI(void)
{
	// 1) Original Engine -> Load the Proxy dllEntry (store the original server's systemCall function pointer in originalSystemCall)
	// 2) The Proxy get the Original dllEntry
	// 3) The Proxy send our own Proxy systemCall function pointer to the Original dllEntry
	// 4) If there is a call to our Proxy's systemCall function it will call the Original systemCall function at the end of it
	proxy.originalGame.dllEntry = (dllEntryFuncPtr_t)YBEProxy_GetFunctionAddress(proxy.originalGame.handle, "dllEntry");

	if (!proxy.originalGame.dllEntry)
	{
		proxy.trap->Print("================================================================\n");
		proxy.trap->Print("----- Proxy: Failed to find dllEntry(), exiting\n");
		proxy.trap->Print("================================================================\n");

		exit(EXIT_FAILURE);
	}

	// Engine -> Proxy vmMain -> Original vmMain
	proxy.originalGame.vmMain = (vmMainFuncPtr_t)YBEProxy_GetFunctionAddress(proxy.originalGame.handle, "vmMain");

	if (!proxy.originalGame.vmMain)
	{
		proxy.trap->Print("================================================================\n");
		proxy.trap->Print("----- Proxy: Failed to find vmMain(), exiting\n");
		proxy.trap->Print("================================================================\n");

		exit(EXIT_FAILURE);
	}

	// "Send our own Proxy systemCall function pointer to the Original dllEntry"
	proxy.originalGame.dllEntry(Proxy_OriginalAPI_VM_DllSyscall);
}

void Proxy_GameLibrary_Load(void)
{
	proxy.trap->Print("----- Proxy: Loading original game library %s\n", PROXY_LIBRARY_NAME PROXY_LIBRARY_DOT PROXY_LIBRARY_EXT);

	Proxy_LoadGameLibrary();

	Proxy_GetOriginalGameAPI();

	proxy.trap->Print("----- Proxy: %s properly loaded\n", PROXY_LIBRARY_NAME PROXY_LIBRARY_DOT PROXY_LIBRARY_EXT);
}

void Proxy_GameLibrary_Unload(void)
{
	proxy.trap->Print("----- Proxy: Unloading original game library %s\n", PROXY_LIBRARY_NAME PROXY_LIBRARY_DOT PROXY_LIBRARY_EXT);

	// We can close the original game library
	YBEProxy_CloseLibrary(proxy.originalGame.handle);

	proxy.trap->Print("----- Proxy: %s properly unloaded\n", PROXY_LIBRARY_NAME PROXY_LIBRARY_DOT PROXY_LIBRARY_EXT);
}
