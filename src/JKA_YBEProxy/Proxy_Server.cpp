#include "Proxy_Server.hpp"
#include "Proxy_Header.hpp"
#include "EnginePatch/Proxy_EnginePatch.hpp"

ProxyServer_t server = { 0 };

void Proxy_Server_Initialize_MemoryAddress(void)
{
	// ----------- SERVER

	// vars
	server.svs = (serverStatic_t*)var_svs_addr;
	server.sv = (server_t*)var_sv_addr;

	// cvars
	server.cvars.sv_fps = *(cvar_t**)cvar_sv_fps_addr;
	server.cvars.sv_gametype = *(cvar_t**)cvar_sv_gametype_addr;
	server.cvars.sv_hostname = *(cvar_t**)cvar_sv_hostname_addr;
	server.cvars.sv_mapname = *(cvar_t**)cvar_sv_mapname_addr;
	server.cvars.sv_maxclients = *(cvar_t**)cvar_sv_maxclients_addr;
	server.cvars.sv_privateClients = *(cvar_t**)cvar_sv_privateClients_addr;
	server.cvars.sv_pure = *(cvar_t**)cvar_sv_pure_addr;
	server.cvars.sv_maxRate = *(cvar_t**)cvar_sv_maxRate_addr;
	server.cvars.sv_rconPassword = *(cvar_t**)cvar_sv_rconPassword_addr;
	server.cvars.sv_floodProtect = *(cvar_t**)cvar_sv_floodProtect_addr;
	server.cvars.sv_allowDownload = *(cvar_t**)cvar_sv_allowDownload_addr;

	// functions
	server.functions.SV_ClientEnterWorld = (void (*)(client_t*, usercmd_t*))func_SV_ClientEnterWorld_addr;
	server.functions.SV_ClientThink = (void (*)(client_t*, usercmd_t*))func_SV_ClientThink_addr;
	server.functions.SV_DropClient = (void (*)(client_t*, const char*))func_SV_DropClient_addr;
	server.functions.SV_Netchan_Transmit = (void (*)(client_t*, msg_t*))func_SV_Netchan_Transmit_addr;
	//server.functions.SV_RateMsec = (int (*)(client_t*, int))func_SV_RateMsec_addr; // we directly use our own function inside "Proxy_sv_snapshot()"
	server.functions.SV_UpdateServerCommandsToClient = (void (*)(client_t*, msg_t*))func_SV_UpdateServerCommandsToClient_addr;
	server.functions.SV_FlushRedirect = (void (*)(char*))func_SV_FlushRedirect_addr;
	server.functions.SV_ExecuteClientCommand = (void (*)(client_t*, const char*, qboolean))func_SV_ExecuteClientCommand_addr;
	server.functions.SV_GetChallenge = (void (*)(netadr_t))func_SV_GetChallenge_addr;
	server.functions.SV_DirectConnect = (void (*)(netadr_t))func_SV_DirectConnect_addr;
	server.functions.SVC_RemoteCommand = (void (*)(netadr_t, msg_t*))func_SVC_RemoteCommand_addr;
	server.functions.SV_Netchan_Process = (qboolean (*)(client_t*, msg_t*))func_SV_Netchan_Process_addr;
	server.functions.SV_SendMessageToClient = (void (*)(msg_t*, client_t*))func_SV_SendMessageToClient_addr;
	server.functions.SVC_Status = (void (*)(netadr_t))func_SVC_Status_addr;
	server.functions.SVC_Info = (void (*)(netadr_t))func_SVC_Info_addr;
	server.functions.SV_ConnectionlessPacket = (void (*)(netadr_t, msg_t*))func_SV_ConnectionlessPacket_addr;
	server.functions.SV_ExecuteClientMessage = (void (*)(client_t*, msg_t*))func_SV_ExecuteClientMessage_addr;
	server.functions.SV_SendClientGameState = (void (*)(client_t*))func_SV_SendClientGameState_addr;
	server.functions.SV_SendServerCommand = (void (QDECL *)(client_t*, const char*, ...))func_SV_SendServerCommand_addr;
	server.functions.SV_UserinfoChanged = (void (*)(client_t*))func_SV_UserinfoChanged_addr;
	server.functions.SV_SendClientSnapshot = (void (*)(client_t*))func_SV_SendClientSnapshot_addr;

	// ----------- COMMON

	// vars
	server.common.vars.logfile = (fileHandle_t*)var_common_logfile_addr;
	server.common.vars.rd_buffer = (char**)var_common_rd_buffer_addr;
	server.common.vars.rd_buffersize = (int*)var_common_rd_buffersize_addr;
	server.common.vars.rd_flush = (void (**)(char*))var_common_rd_flush_addr;
	server.common.vars.cmd_argc = (int*)var_cmd_argc_addr;
	server.common.vars.cmd_argv = (char* (*)[MAX_STRING_TOKENS])var_cmd_argv_addr;
	server.common.vars.cmd_tokenized = (char (*)[BIG_INFO_STRING + MAX_STRING_TOKENS])var_cmd_tokenized_addr;

	// cvars
	server.common.cvars.com_dedicated = *(cvar_t**)cvar_common_com_dedicated_addr;
	server.common.cvars.com_sv_running = *(cvar_t**)cvar_common_com_sv_running_addr;
	server.common.cvars.com_cl_running = *(cvar_t**)cvar_common_com_cl_running_addr;
	server.common.cvars.com_logfile = *(cvar_t**)cvar_common_com_logfile_addr;
	server.common.cvars.com_developer = *(cvar_t**)cvar_common_com_developer_addr;
	server.common.cvars.fs_gamedirvar = *(cvar_t**)cvar_common_fs_gamedirvar_addr;

	// functions
	server.common.functions.Com_DPrintf = (void (QDECL*)(const char*, ...))func_Com_DPrintf_addr;
	server.common.functions.Com_HashKey = (int (*)(char*, int))func_Com_HashKey_addr;
	server.common.functions.Com_Printf = (void (QDECL *)(const char*, ...))func_Com_Printf_addr; // It will call the fixed one in Proxy_files
	server.common.functions.Com_BeginRedirect = (void (*)(char*, int, void (*)(char*)))func_Com_BeginRedirect_addr;
	server.common.functions.Com_EndRedirect = (void (*)(void))func_Com_EndRedirect_addr;
	server.common.functions.Cvar_VariableString = (char* (*)(const char*))func_Cvar_VariableString_addr;
	server.common.functions.Cvar_VariableValue = (float (*)(const char*))func_Cvar_VariableValue_addr;
	server.common.functions.FS_FOpenFileWrite = (fileHandle_t (*)(const char*))func_FS_FOpenFileWrite_addr;
	server.common.functions.FS_ForceFlush = (void (*)(fileHandle_t))func_FS_ForceFlush_addr;
	server.common.functions.FS_Initialized = (qboolean(*)())func_FS_Initialized_addr;
	server.common.functions.FS_Write = (int (*)(const void*, int, fileHandle_t))func_FS_Write_addr;
	server.common.functions.FS_FileIsInPAK = (int (*)(const char*, int*))func_FS_FileIsInPAK_addr;
	server.common.functions.FS_LoadedPakPureChecksums = (const char* (*)(void))func_FS_LoadedPakPureChecksums_addr;
	server.common.functions.FS_ReferencedPakNames = (const char* (*)(void))func_FS_ReferencedPakNames_addr;
	server.common.functions.FS_SV_FOpenFileRead = (int (*)(const char*, fileHandle_t*))func_FS_SV_FOpenFileRead_addr;
	server.common.functions.FS_Read = (int (*)(void*, int, fileHandle_t))func_FS_Read_addr;
	server.common.functions.FS_FCloseFile = (void (*)(fileHandle_t))func_FS_FCloseFile_addr;
	server.common.functions.Netchan_TransmitNextFragment = (void (*)(netchan_t*))func_Netchan_TransmitNextFragment_addr;
	server.common.functions.NET_AdrToString = (const char* (*)(netadr_t))func_NET_AdrToString_addr;
	server.common.functions.NET_OutOfBandPrint = (void (QDECL*)(netsrc_t, netadr_t, const char*, ...))func_NET_OutOfBandPrint_addr;
	server.common.functions.NET_CompareBaseAdr = (qboolean (*)(netadr_t, netadr_t))func_NET_CompareBaseAdr_addr;
	server.common.functions.MSG_Init = (void (*)(msg_t*, byte*, int))func_MSG_Init_addr;
	server.common.functions.MSG_ReadByte = (int (*)(msg_t*))func_MSG_ReadByte_addr;
	server.common.functions.MSG_ReadDeltaUsercmdKey = (void (*)(msg_t*, int, usercmd_t*, usercmd_t*))func_MSG_ReadDeltaUsercmdKey_addr;
	server.common.functions.MSG_ReadShort = (int (*)(msg_t*))func_MSG_ReadShort_addr;
	server.common.functions.MSG_ReadLong = (int (*)(msg_t*))func_MSG_ReadLong_addr;
	server.common.functions.MSG_ReadString = (char* (*)(msg_t*))func_MSG_ReadString_addr;
	server.common.functions.MSG_ReadStringLine = (char* (*)(msg_t*))func_MSG_ReadStringLine_addr;
	server.common.functions.MSG_Bitstream = (void (*)(msg_t*))func_MSG_Bitstream_addr;
	server.common.functions.MSG_BeginReadingOOB = (void (*)(msg_t*))func_MSG_BeginReadingOOB_addr;
	server.common.functions.MSG_WriteBigString = (void (*)(msg_t*, const char*))func_MSG_WriteBigString_addr;
	server.common.functions.MSG_WriteByte = (void (*)(msg_t*, int))func_MSG_WriteByte_addr;
	server.common.functions.MSG_WriteDeltaEntity = (void (*)(msg_t*, struct entityState_s*, struct entityState_s*, qboolean))func_MSG_WriteDeltaEntity_addr;
	server.common.functions.MSG_WriteLong = (void (*)(msg_t*, int))func_MSG_WriteLong_addr;
	server.common.functions.MSG_WriteShort = (void (*)(msg_t*, int))func_MSG_WriteShort_addr;
	server.common.functions.MSG_WriteString = (void (*)(msg_t*, const char*))func_MSG_WriteString_addr;
	server.common.functions.MSG_WriteData = (void (*)(msg_t*, const void*, int))func_MSG_WriteData_addr;
	server.common.functions.Sys_IsLANAddress = (qboolean(*)(netadr_t))func_Sys_IsLANAddress_addr;
	server.common.functions.Sys_Print = (void (*)(const char*))func_Sys_Print_addr;
	server.common.functions.Cmd_Argv = (char* (*)(int))func_Cmd_Argv_addr;
	server.common.functions.Cmd_ExecuteString = (void (*)(const char*))func_Cmd_ExecuteString_addr;
	server.common.functions.Cmd_TokenizeString = (void (*)(const char*))func_Cmd_TokenizeString_addr;
	server.common.functions.Huff_Decompress = (void (*)(msg_t*, int))func_Huff_Decompress_addr;
	server.common.functions.Z_Free = (void (*)(void*))func_Z_Free_addr;
	server.common.functions.Z_Malloc = (void* (*)(int, memtag_t, qboolean, int))func_Z_Malloc_addr;
}

void Proxy_Server_UpdateUcmdStats(int clientNum, usercmd_t* cmd, int packetIndex)
{
	Proxy_s::ClientData_s* currentClientData = &proxy.clientData[clientNum];

	currentClientData->cmdIndex++;
	const int cmdIndex = currentClientData->cmdIndex & (CMD_MASK - 1);
	currentClientData->cmdStats[cmdIndex].serverTime = cmd->serverTime;
	currentClientData->cmdStats[cmdIndex].packetIndex = packetIndex;
}

// Update value of packets and FPS
// From JK2MV (updated version by fau)
void Proxy_Server_CalcPacketsAndFPS(int clientNum, int* packets, int* fps)
{
	int		lastCmdTime;
	int		lastPacketIndex = 0;
	int		i;

	Proxy_s::ClientData_s *currentClientData = &proxy.clientData[clientNum];

	lastCmdTime = currentClientData->cmdStats[currentClientData->cmdIndex & (CMD_MASK - 1)].serverTime;

	for (i = 0; i < CMD_MASK; i++)
	{
		ucmdStat_t* stat = &currentClientData->cmdStats[i];

		if (stat->serverTime + 1000 >= lastCmdTime)
		{
			(*fps)++;

			if (lastPacketIndex != stat->packetIndex)
			{
				lastPacketIndex = stat->packetIndex;
				(*packets)++;
			}
		}
	}
}

// Some clientside timings (delta or whatever) that we can't calc here (I think)
static inline int Proxy_GetTimenudgeMagicOffset(int svFps)
{
#if defined(_WIN32) && !defined(MINGW32)
	constexpr int magicOffsets[] = {
		21, 18, 18, 17, 18, 16, 14, 13, 12, 13, // [20, 29]
		13, 20, 10, 9, 9, 9, 8, 9, 9, 9,		// [30, 39]
		9, 5, 6, 6, 5, 5, 6, 6, 5, 5, 			// [40, 49]
		5, 5, 5, 4, 4, 4, 5, 5, 5, 3, 			// [50, 59]
		0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 			// [60, 69] DO NOT USE 60, 61, 62
		1, 0, 0, 0, 0, 0, 0, 0, 0, 0,			// [70, 79]
		0										// [80[
	};
#else
	constexpr int magicOffsets[] = {
		18, 15, 15, 13, 14, 14, 11, 10, 9, 10,				// [20, 29]
		10, 0, 0, 0, 0, -21, -21, -21, -21, -19,			// [30, 39] DO NOT USE 31, 32, 33, 34
		-19, 0, 0, 0, 0, -19, -17, -17, -16, -17, 			// [40, 49] DO NOT USE 41, 42, 43, 44
		-17, -17, -17, -17, -17, -17, -16, -15, -15, 0,		// [50, 59] DO NOT USE 59
		0, 0, 0, -30, -30, -30, -30, -28, -28, -28, 		// [60, 69] DO NOT USE 60, 61, 62
		-28, -28, -28, -28, -28, -28, -27, -26, -26, -27,	// [70, 79]
		-27													// [80[
	};
#endif
	constexpr int tableSize = sizeof(magicOffsets) / sizeof(magicOffsets[0]);

	int index = 0;
	int magicOffset = 0;
	
	if (svFps >= 20)
	{
		index = svFps - 20; // the first offset starts at sv_fps 20, so just -20 to get the index

		if (svFps >= tableSize + 20)
		{
			index = tableSize - 1; // last index of the tab, the offset should be 0 if sv_fps greater than 80
		}

		magicOffset = magicOffsets[index];
	}

	return magicOffset;
}

void Proxy_Server_UpdateTimenudge(client_t* client, usercmd_t* cmd, int _Milliseconds)
{
	Proxy_s::ClientData_s *currentClientData = &proxy.clientData[getClientNumFromAddr(client)];

	currentClientData->timenudgeData.delayCount++;
	currentClientData->timenudgeData.delaySum += cmd->serverTime - server.svs->time;
	currentClientData->timenudgeData.pingSum += client->ping;

	// Wait 1000 ms so we have enough data when we'll calcul an approximation of the timenudge
	if (_Milliseconds < currentClientData->timenudgeData.lastTimeTimeNudgeCalculation + 1500)
	{
		return;
	}

	const int svFps = server.cvars.sv_fps->integer;
	const int magicOffset = Proxy_GetTimenudgeMagicOffset(svFps);

	// ((serverTime - sv.time) + ping -magicOffset + (1000/sv_fps)) * -1
	currentClientData->timenudge =
		(
			(currentClientData->timenudgeData.delaySum / (float)currentClientData->timenudgeData.delayCount)
			+ (currentClientData->timenudgeData.pingSum / (float)currentClientData->timenudgeData.delayCount)
			- magicOffset
			+ (1000 / svFps)
		) * -1;

	currentClientData->timenudgeData.delayCount = 0;
	currentClientData->timenudgeData.delaySum = 0;
	currentClientData->timenudgeData.pingSum = 0;

	currentClientData->timenudgeData.lastTimeTimeNudgeCalculation = _Milliseconds;
}