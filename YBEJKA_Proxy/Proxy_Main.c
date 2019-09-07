#include "Proxy_Header.h"

Proxy_t proxy = { 0 };

__declspec(dllexport) int vmMain(int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11)
{
	switch (command)
	{
		case GAME_INIT:
		{
			// (int levelTime, int randomSeed, int restart)

			proxy.jampgameHandle = LoadLibrary(L"Base/YBEJKA_Proxy.dll");

			if (proxy.jampgameHandle == NULL)
			{
				exit(EXIT_FAILURE);
			}
			else
			{
				proxy.originalDllEntry = (dllEntryFuncPtr_t)GetProcAddress(proxy.jampgameHandle, "dllEntry");
				proxy.originalVmMain = (vmMainFuncPtr_t) GetProcAddress(proxy.jampgameHandle, "vmMain");

				proxy.originalDllEntry(proxy.originalSystemCall);
			}

			break;
		}
		case GAME_SHUTDOWN:
		{
			//(int restart)

			if (proxy.jampgameHandle)
			{
				proxy.originalVmMainResponse = proxy.originalVmMain(command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);
				FreeLibrary(proxy.jampgameHandle);

				return proxy.originalVmMainResponse;
			}

			break;
		}
		case GAME_CLIENT_CONNECT:
		{
			//(int clientNum, qboolean firstTime, qboolean isBot)

			if ((qboolean)arg1 && !(qboolean)arg2)
			{
				trap_SendServerCommand(arg0, "print \"Proxy Anticheat par Yberion\"");
			}
		}
		default:
			break;
	}

	return proxy.originalVmMain(command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);
}

// The engine sends the system call function pointer the the game module through dllEntry
__declspec(dllexport) void dllEntry(systemCallFuncPtr_t systemCallFuncPtdr)
{
	proxy.originalSystemCall = systemCallFuncPtdr;
}