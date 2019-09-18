#include "Proxy_Header.h"

void Proxy_LoadOriginalGameLibrary(void)
{
	// Todo: Check for fs_basegame
	char fs_gameBuffer[MAX_OSPATH];
	size_t pathLength = 0;
	size_t tmpLength = 0;

	fs_gameBuffer[0] = 0;

	proxy.trap->Cvar_VariableStringBuffer(FS_GAME_CVAR, fs_gameBuffer, sizeof(fs_gameBuffer));

	pathLength = strlen(fs_gameBuffer);

	// length 0 means that fs_game is set to the default value which is the default base game folder name
	if (pathLength == 0)
	{
		pathLength += strlen(DEFAULT_BASE_GAME_FOLDER_NAME);
		memcpy(fs_gameBuffer, DEFAULT_BASE_GAME_FOLDER_NAME, pathLength);
	}

	tmpLength = strlen(PROXY_LIBRARY);
	memcpy(fs_gameBuffer + pathLength, PROXY_LIBRARY, tmpLength);
	pathLength += tmpLength;

	fs_gameBuffer[pathLength] = '\0';

	proxy.jampgameHandle = YBEProxy_OpenLibrary(fs_gameBuffer);

	if (proxy.jampgameHandle == NULL)
	{
		exit(EXIT_FAILURE);
	}
}