#include "JKA_YBEProxy/Proxy.hpp"
#include "JKA_YBEProxy/Module/GameLibrary.hpp"

void Proxy_LoadGameLibrary(void)
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

	proxy.jampgameHandle = YBEProxy_OpenLibrary(libraryPath);

	// exit() as the engine does when it can't load a game module (ERR_FATAL): no map can run without it
	if (proxy.jampgameHandle == NULL)
	{
		proxy.trap->Print("================================================================\n");
		proxy.trap->Print("----- Proxy: Failed to open %s, exiting\n", libraryPath);
		proxy.trap->Print("================================================================\n");

		exit(EXIT_FAILURE);
	}
}
