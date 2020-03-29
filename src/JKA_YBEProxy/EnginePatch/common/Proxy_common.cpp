#include "JKA_YBEProxy/EnginePatch/Proxy_EnginePatch.hpp"

const char* FS_GetCurrentGameDir(bool emptybase)
{
	if (server.common.cvars.fs_gamedirvar->string[0])
		return server.common.cvars.fs_gamedirvar->string;

	return emptybase ? "" : BASEGAME;
}