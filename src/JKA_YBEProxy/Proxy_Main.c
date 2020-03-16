// ==================================================
// JKA_YBEProxy by Yberion
// Inspired by JMPProxy from DeathSpike (https://github.com/Deathspike/JMPProxy)
// ==================================================

#include "Proxy_Header.h"

Proxy_t proxy = { 0 };

static void Proxy_OldAPI_Init(void)
{
	Proxy_LoadOriginalGameLibrary();

	// Engine -> Load Proxy dllEntry (store the original systemCall function pointer in originalSystemCall) ->
	// Get Original dllEntry -> send our own Proxy systemCall function pointer to the Original dllEntry
	// At the end of our Proxy systemCall function there's a call the Original systemCall function
	proxy.originalDllEntry = (dllEntryFuncPtr_t)YBEProxy_GetFunctionAddress(proxy.jampgameHandle, "dllEntry");
	// Engine -> Proxy vmMain -> Original vmMain
	proxy.originalVmMain = (vmMainFuncPtr_t)YBEProxy_GetFunctionAddress(proxy.jampgameHandle, "vmMain");

	// "Send our own Proxy systemCall function pointer to the Original dllEntry"
	proxy.originalDllEntry(Proxy_OldAPI_systemCall);
}

Q_EXPORT intptr_t vmMain(intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4,
	intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11)
{
	switch (command)
	{
		//==================================================
		case GAME_INIT: // (int levelTime, int randomSeed, int restart)
		//==================================================
		{
			Proxy_OldAPI_Init();

			// Check version oh jampded here
			Proxy_Patch_Attach();

			break;
		}
		//==================================================
		case GAME_SHUTDOWN: // (int restart)
		//==================================================
		{
			// Check version oh jampded here
			Proxy_Patch_Detach();

			if (proxy.jampgameHandle)
			{
				// Send the shutdown signal to the original game module and store the response
				proxy.originalVmMainResponse = proxy.originalVmMain(command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);

				// We can close our proxy library
				YBEProxy_CloseLibrary(proxy.jampgameHandle);

				// Return the response of the original game module after the shutdown
				return proxy.originalVmMainResponse;
			}
			break;
		}
		//==================================================
		case GAME_CLIENT_CONNECT: // (int clientNum, qboolean firstTime, qboolean isBot)
		//==================================================
		{
			Proxy_SharedAPI_ClientConnect(arg0, (qboolean)arg1, (qboolean)arg2);

			break;
		}
		//==================================================
		case GAME_CLIENT_BEGIN: // (int clientNum, qboolean allowTeamReset)
		//==================================================
		{
			Proxy_SharedAPI_ClientBegin(arg0, (qboolean)arg1);

			break;
		}
		//==================================================
		case GAME_CLIENT_COMMAND: // (int clientNum)
		//==================================================
		{
			if (!Proxy_SharedAPI_ClientCommand(arg0))
			{
				return 0;
			}

			break;
		}
		//==================================================
		case GAME_CLIENT_THINK: // (int clientNum)
		//==================================================
		{
			int response = proxy.originalVmMain(command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);

			Proxy_SharedAPI_ClientThink((int)arg0, (usercmd_t*)arg1);

			return response;
		}
		//==================================================
		case GAME_CLIENT_USERINFO_CHANGED: // (int clientNum)
		//==================================================
		{
			Proxy_SharedAPI_ClientUserinfoChanged((int)arg0);

			break;
		}
		default:
			break;
	}

	return proxy.originalVmMain(command, arg0, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10, arg11);
}

// The engine sends the system call function pointer the the game module through dllEntry
Q_EXPORT void dllEntry(systemCallFuncPtr_t systemCallFuncPtdr) {
	proxy.originalSystemCall = systemCallFuncPtdr;

	// Create trap calls available directly within the proxy
	// this way we can use trap_ calls
	TranslateSystemCalls();
}

Q_EXPORT gameExport_t* QDECL GetModuleAPI(int apiVersion, gameImport_t* import)
{
	assert(import);

	// Needed for trap_... calls inside of the proxy
	proxy.trap = import;

	if (apiVersion != GAME_API_VERSION)
	{
		proxy.trap->Print("Mismatched GAME_API_VERSION: expected %i, got %i\n", GAME_API_VERSION, apiVersion);
		return NULL;
	}

	static gameImport_t copyNewAPIGameImportTable_ = { 0 };
	static gameExport_t copyNewAPIGameExportTable_ = { 0 };

	proxy.originalNewAPIGameImportTable = import;
	memcpy(&copyNewAPIGameImportTable_, import, sizeof(gameImport_t));
	proxy.copyNewAPIGameImportTable = &copyNewAPIGameImportTable_;

	Proxy_LoadOriginalGameLibrary();

	GetGameAPI_t jampGameGetModuleAPI = (GetGameAPI_t)YBEProxy_GetFunctionAddress(proxy.jampgameHandle, "GetModuleAPI");
	
	proxy.originalNewAPIGameExportTable = jampGameGetModuleAPI(apiVersion, &copyNewAPIGameImportTable_);
	memcpy(&copyNewAPIGameExportTable_, proxy.originalNewAPIGameExportTable, sizeof(gameExport_t));
	proxy.copyNewAPIGameExportTable = &copyNewAPIGameExportTable_;

	Proxy_NewAPI_InitLayerExportTable();
	Proxy_NewAPI_InitLayerImportTable();

	return proxy.copyNewAPIGameExportTable;
}