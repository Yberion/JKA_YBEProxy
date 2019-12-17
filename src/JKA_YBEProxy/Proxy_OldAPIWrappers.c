#include "Proxy_Header.h"

intptr_t QDECL Proxy_OldAPI_systemCall(intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5,
	intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11, intptr_t arg12, intptr_t arg13, intptr_t arg14)
{
	switch (command)
	{
		//==================================================
		case G_LOCATE_GAME_DATA: // (sharedEntity_t* gEnts, int numGEntities, int sizeofGEntity_t, playerState_t* clients, int sizeofGameClient)
		//==================================================
		{
			Proxy_SharedAPI_LocateGameData((sharedEntity_t*)arg0, (int)arg1, (int)arg2, (playerState_t*)arg3, (int)arg4);

			break;
		}
		//==================================================
		case G_GET_USERCMD: // (int clientNum, usercmd_t* cmd)
		//==================================================
		{
			int response = proxy.originalSystemCall(command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14);
			
			Proxy_SharedAPI_GetUsercmd((int)arg0, (usercmd_t*)arg1);
			
			return response;
		}
		default:
			break;
	}

	return proxy.originalSystemCall(command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11, arg12, arg13, arg14);
}