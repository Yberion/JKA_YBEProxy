#include "Proxy_Header.hpp"

intptr_t QDECL Proxy_OldAPI_SystemCall(intptr_t command, ...)
{
	intptr_t args[15];
	
	va_list ap;

	va_start(ap, command);

	for (size_t i = 0; i < sizeof(args) / sizeof(args[i]); ++i)
	{
		args[i] = va_arg(ap, intptr_t);
	}

	va_end(ap);

	switch (command)
	{
		//==================================================
		case G_LOCATE_GAME_DATA: // (sharedEntity_t* gEnts, int numGEntities, int sizeofGEntity_t, playerState_t* clients, int sizeofGameClient)
		//==================================================
		{
			Proxy_SharedAPI_LocateGameData((sharedEntity_t*)args[0], (int)args[1], (int)args[2], (playerState_t*)args[3], (int)args[4]);

			break;
		}
		//==================================================
		case G_GET_USERCMD: // (int clientNum, usercmd_t* cmd)
		//==================================================
		{
			int response = proxy.originalSystemCall(command, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14]);
			
			Proxy_SharedAPI_GetUsercmd((int)args[0], (usercmd_t*)args[1]);
			
			return response;
		}
		default:
			break;
	}

	return proxy.originalSystemCall(command, args[0], args[1], args[2], args[3], args[4], args[5], args[6], args[7], args[8], args[9], args[10], args[11], args[12], args[13], args[14]);
}