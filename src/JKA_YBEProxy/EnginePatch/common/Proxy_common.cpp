#include "JKA_YBEProxy/EnginePatch/Proxy_EnginePatch.hpp"

const char* FS_GetCurrentGameDir(bool emptybase)
{
	if (proxy.server.common.fs_gamedirvar->string[0])
		return proxy.server.common.fs_gamedirvar->string;

	return emptybase ? "" : BASEGAME;
}