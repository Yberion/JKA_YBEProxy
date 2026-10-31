// ==================================================
// JKA_YBEProxy by Yberion
// Inspired by JMPProxy from DeathSpike (https://github.com/Deathspike/JMPProxy)
// ==================================================

#include "JKA_YBEProxy/Module/Main.hpp"
#include "JKA_YBEProxy/Proxy.hpp"
#include "JKA_YBEProxy/Module/EngineApi.hpp"
#include "JKA_YBEProxy/Module/GameExports.hpp"
#include "JKA_YBEProxy/Module/GameLibrary.hpp"

// The engine calls the game module here. The proxy runs its part of each command, then forwards the command to the
// original game module, whose response goes back to the engine
Q_CABI Q_EXPORT intptr_t vmMain(intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4,
	intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10)
{
	switch (command)
	{
		//==================================================
		case GAME_INIT: // (int levelTime, int randomSeed, int restart)
		//==================================================
		{
			Proxy_SharedAPI_Init();

			const intptr_t response = proxy.originalGame.vmMain(command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);

			Proxy_SharedAPI_InitDone();

			return response;
		}
		//==================================================
		case GAME_SHUTDOWN: // (int restart)
		//==================================================
		{
			Proxy_SharedAPI_Shutdown();

			if (proxy.originalGame.handle)
			{
				// Send the shutdown signal to the original game module and store the response
				const intptr_t response = proxy.originalGame.vmMain(command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);

				Proxy_GameLibrary_Unload();

				// Return the response of the original game module after the shutdown
				return response;
			}

			break;
		}
		//==================================================
		case GAME_RUN_FRAME: // (int levelTime)
		//==================================================
		{
			Proxy_SharedAPI_RunFrame((int)arg0);

			break;
		}
		//==================================================
		case GAME_CLIENT_CONNECT: // (int clientNum, qboolean firstTime, qboolean isBot)
		//==================================================
		{
			const char* reason = Proxy_SharedAPI_ClientConnect((int)arg0, (qboolean)arg1, (qboolean)arg2);

			// The engine refuses the client with this reason, as with the game's "Banned."
			if (reason)
			{
				return (intptr_t)reason;
			}

			const intptr_t response = proxy.originalGame.vmMain(command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);

			// The game returns a reason when it refuses the client
			if (!response)
			{
				Proxy_SharedAPI_ClientConnected((int)arg0, (qboolean)arg2);
			}

			return response;
		}
		//==================================================
		case GAME_CLIENT_DISCONNECT: // (int clientNum, qboolean firstTime, qboolean isBot)
		//==================================================
		{
			Proxy_SharedAPI_ClientDisconnect((int)arg0);

			break;
		}
		//==================================================
		case GAME_CLIENT_BEGIN: // (int clientNum, qboolean allowTeamReset)
		//==================================================
		{
			Proxy_SharedAPI_ClientBegin((int)arg0, (qboolean)arg1);

			break;
		}
		//==================================================
		case GAME_CLIENT_COMMAND: // (int clientNum)
		//==================================================
		{
			if (!Proxy_SharedAPI_ClientCommand((int)arg0))
			{
				return 0;
			}

			const intptr_t response = proxy.originalGame.vmMain(command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);

			Proxy_SharedAPI_ClientCommandDone();

			return response;
		}
		/*
		//==================================================
		case GAME_CLIENT_THINK: // (int clientNum)
		//==================================================
		{
			int response = proxy.originalGame.vmMain(command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);

			//Proxy_SharedAPI_ClientThink((int)arg0, (usercmd_t*)arg1);

			return response;
		}
		*/
		//==================================================
		case GAME_CONSOLE_COMMAND: // (void)
		//==================================================
		{
			if (Proxy_SharedAPI_ConsoleCommand())
			{
				return qtrue;
			}

			break;
		}
		//==================================================
		case GAME_CLIENT_USERINFO_CHANGED: // (int clientNum)
		//==================================================
		{
			// The client got dropped: don't forward, the game would set CS_PLAYERS again for a gone client (ghost player)
			if (!Proxy_SharedAPI_ClientUserinfoChanged((int)arg0))
			{
				return 0;
			}

			break;
		}
		default:
			break;
	}

	return proxy.originalGame.vmMain(command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
}

// The engine sends the system call function pointer to the game module through dllEntry
Q_CABI Q_EXPORT void dllEntry(systemCallFuncPtr_t systemCallFuncPtdr) {
	proxy.originalSystemCall = systemCallFuncPtdr;

	// Create trap calls available directly within the proxy (proxy.trap->*)
	Proxy_Translate_SystemCalls();
}

// No GetModuleAPI export: OpenJK then loads the proxy through dllEntry and vmMain (legacy API), as the original engine does
