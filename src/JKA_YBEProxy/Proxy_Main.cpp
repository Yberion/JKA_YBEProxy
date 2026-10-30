// ==================================================
// JKA_YBEProxy by Yberion
// Inspired by JMPProxy from DeathSpike (https://github.com/Deathspike/JMPProxy)
// ==================================================

#include "JKA_YBEProxy/Proxy_Main.hpp"
#include "JKA_YBEProxy/Proxy_Header.hpp"
#include "JKA_YBEProxy/Proxy_CVars.hpp"
#include "JKA_YBEProxy/Proxy_Files.hpp"
#include "JKA_YBEProxy/Proxy_Bans.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_NetStatus.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Patch.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Restart.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Wrappers.hpp"
#include "JKA_YBEProxy/Proxy_SharedAPI.hpp"
#include "JKA_YBEProxy/Proxy_Translate_SystemCalls.hpp"
#include "JKA_YBEProxy/Wrappers/Proxy_OriginalAPI_Wrappers.hpp"

Proxy_t proxy = {};

static void Proxy_GetOriginalGameAPI(void)
{
	// 1) Original Engine -> Load the Proxy dllEntry (store the original server's systemCall function pointer in originalSystemCall)
	// 2) The Proxy get the Original dllEntry
	// 3) The Proxy send our own Proxy systemCall function pointer to the Original dllEntry
	// 4) If there is a call to our Proxy's systemCall function it will call the Original systemCall function at the end of it
	proxy.originalDllEntry = (dllEntryFuncPtr_t)YBEProxy_GetFunctionAddress(proxy.jampgameHandle, "dllEntry");

	if (!proxy.originalDllEntry)
	{
		proxy.trap->Print("================================================================\n");
		proxy.trap->Print("----- Proxy: Failed to find dllEntry(), exiting\n");
		proxy.trap->Print("================================================================\n");

		exit(EXIT_FAILURE);
	}

	// Engine -> Proxy vmMain -> Original vmMain
	proxy.originalVmMain = (vmMainFuncPtr_t)YBEProxy_GetFunctionAddress(proxy.jampgameHandle, "vmMain");

	if (!proxy.originalVmMain)
	{
		proxy.trap->Print("================================================================\n");
		proxy.trap->Print("----- Proxy: Failed to find vmMain(), exiting\n");
		proxy.trap->Print("================================================================\n");

		exit(EXIT_FAILURE);
	}

	// "Send our own Proxy systemCall function pointer to the Original dllEntry"
	proxy.originalDllEntry(Proxy_OriginalAPI_VM_DllSyscall);
}

Q_CABI Q_EXPORT intptr_t vmMain(intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4,
	intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10)
{
	switch (command)
	{
		//==================================================
		case GAME_INIT: // (int levelTime, int randomSeed, int restart)
		//==================================================
		{
			proxy.trap->Print("================================================================\n");
			proxy.trap->Print("----- Proxy: %s %s %s\n", YBEPROXY_NAME, YBEPROXY_VERSION, YBEPROXY_BY_AUTHOR);
			proxy.trap->Print("================================================================\n");

			proxy.trap->Print("----- Proxy: Loading original game library %s\n", PROXY_LIBRARY_NAME PROXY_LIBRARY_DOT PROXY_LIBRARY_EXT);

			Proxy_LoadGameLibrary();

			Proxy_GetOriginalGameAPI();

			proxy.trap->Print("----- Proxy: %s properly loaded\n", PROXY_LIBRARY_NAME PROXY_LIBRARY_DOT PROXY_LIBRARY_EXT);
			
			proxy.trap->Print("----- Proxy: Initializing Proxy CVars\n");

			Proxy_CVars_Registration();

			proxy.trap->Print("----- Proxy: Proxy CVars properly initialized\n");

			char version[MAX_STRING_CHARS];

			proxy.trap->Cvar_VariableStringBuffer("version", version, sizeof(version));

			if (!Q_stricmpn(version, ORIGINAL_ENGINE_VERSION, sizeof(ORIGINAL_ENGINE_VERSION) - 1))
			{
				proxy.isOriginalEngine = true;
			}

			if (proxy.isOriginalEngine)
			{
				proxy.trap->Print("----- Proxy: Original engine detected\n");

				Proxy_Engine_Initialize_MemoryLayer();
				Proxy_OriginalEngine_CVars_Registration();
				
				proxy.trap->Print("----- Proxy: Patching engine\n");

				Proxy_Engine_Inline_Patches();
				Proxy_Engine_Attach_Patches();

				proxy.trap->Print("----- Proxy: Engine properly patched\n");
			}

			// Before the game loads its ban list from g_banIPs
			Proxy_Bans_BeforeGameInit();

			break;
		}
		//==================================================
		case GAME_SHUTDOWN: // (int restart)
		//==================================================
		{
			if (proxy.isOriginalEngine)
			{
				// On "rcon map XXX" or "rcon map_restart 0" it directly goes there from SVC_RemoteCommand,
				// which only calls Com_EndRedirect() once Cmd_ExecuteString() returns, after the whole map load
				// While the redirect is active, Com_Printf skips the console and the log file: end it here,
				// so the rest of the map load is printed there. The later Com_EndRedirect() then does nothing
				server.common.functions.Com_EndRedirect();

				proxy.trap->Print("----- Proxy: Unpatching engine\n");

				Proxy_Engine_Detach_Patches();

				proxy.trap->Print("----- Proxy: Engine properly unpatched\n");
			}

			if (proxy.jampgameHandle)
			{
				// Send the shutdown signal to the original game module and store the response
				proxy.originalVmMainResponse = proxy.originalVmMain(command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);

				proxy.trap->Print("----- Proxy: Unloading original game library %s\n", PROXY_LIBRARY_NAME PROXY_LIBRARY_DOT PROXY_LIBRARY_EXT);

				// We can close the original game library
				YBEProxy_CloseLibrary(proxy.jampgameHandle);

				proxy.trap->Print("----- Proxy: %s properly unloaded\n", PROXY_LIBRARY_NAME PROXY_LIBRARY_DOT PROXY_LIBRARY_EXT);

				// Return the response of the original game module after the shutdown
				return proxy.originalVmMainResponse;
			}

			break;
		}
		// ==================================================
		case GAME_RUN_FRAME:
		// ==================================================
		// The server is going to active its new frame, which
		// often is around 20 times per second. Active some
		// of our own active functions.
		// ==================================================
		{
			Proxy_UpdateAllCvars((int)arg0);

			if (proxy.isOriginalEngine && proxy.originalEngineCvars.proxy_sv_enableNetStatus.integer)
			{
				Proxy_Engine_NetStatus_FrameClock();
			}

			if (proxy.isOriginalEngine)
			{
				Proxy_Engine_Restart_BeforeWrap();
			}

			proxy.proxyData.svsTime = (int)arg0;

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

			const intptr_t response = proxy.originalVmMain(command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);

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

			break;
		}
		/*
		//==================================================
		case GAME_CLIENT_THINK: // (int clientNum)
		//==================================================
		{
			int response = proxy.originalVmMain(command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);

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

	return proxy.originalVmMain(command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
}

// The engine sends the system call function pointer to the game module through dllEntry
Q_CABI Q_EXPORT void dllEntry(systemCallFuncPtr_t systemCallFuncPtdr) {
	proxy.originalSystemCall = systemCallFuncPtdr;

	// Create trap calls available directly within the proxy (proxy.trap->*)
	Proxy_Translate_SystemCalls();
}

// No GetModuleAPI export: OpenJK then loads the proxy through dllEntry and vmMain (legacy API), as the original engine does
