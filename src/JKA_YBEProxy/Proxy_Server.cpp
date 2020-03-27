#include "Proxy_Server.hpp"
#include "Proxy_Header.hpp"

void Proxy_Server_Initialize_MemoryAddress(void)
{
	// SERVER

	proxy.server.svs = (serverStatic_t*)var_svs_addr;
	proxy.server.sv = (server_t*)var_sv_addr;

	proxy.server.cvars.sv_fps = *(cvar_t**)cvar_sv_fps_addr;
	proxy.server.cvars.sv_gametype = *(cvar_t**)cvar_sv_gametype_addr;
	proxy.server.cvars.sv_hostname = *(cvar_t**)cvar_sv_hostname_addr;
	proxy.server.cvars.sv_mapname = *(cvar_t**)cvar_sv_mapname_addr;
	proxy.server.cvars.sv_maxclients = *(cvar_t**)cvar_sv_maxclients_addr;
	proxy.server.cvars.sv_privateClients = *(cvar_t**)cvar_sv_privateClients_addr;
	proxy.server.cvars.sv_pure = *(cvar_t**)cvar_sv_pure_addr;

	proxy.server.functions.SV_ClientEnterWorld = (void (*)(client_t*, usercmd_t*))func_SV_ClientEnterWorld_addr;
	proxy.server.functions.SV_ClientThink = (void (*)(client_t*, usercmd_t*))func_SV_ClientThink_addr;
	proxy.server.functions.SV_DropClient = (void (*)(client_t*, const char*))func_SV_DropClient_addr;
	proxy.server.functions.SV_Netchan_Transmit = (void (*)(client_t*, msg_t*))func_SV_Netchan_Transmit_addr;
	proxy.server.functions.SV_RateMsec = (int (*)(client_t*, int))func_SV_RateMsec_addr;
	proxy.server.functions.SV_UpdateServerCommandsToClient = (void (*)(client_t*, msg_t*))func_SV_UpdateServerCommandsToClient_addr;

	// COMMON

	proxy.server.common.com_dedicated = *(cvar_t**)cvar_common_com_dedicated_addr;
	proxy.server.common.com_sv_running = *(cvar_t**)cvar_common_com_sv_running_addr;
	proxy.server.common.com_logfile = *(cvar_t**)cvar_common_com_logfile_addr;
	proxy.server.common.fs_gamedirvar = *(cvar_t**)cvar_common_fs_gamedirvar_addr;

	proxy.server.common.logfile = (fileHandle_t*)var_common_logfile_addr;
	proxy.server.common.rd_buffer = (char**)var_common_rd_buffer_addr;
	proxy.server.common.rd_buffersize = (int*)var_common_rd_buffersize_addr;

	proxy.server.common.Com_DPrintf = (void (QDECL*)(const char*, ...))func_Com_DPrintf_addr;
	proxy.server.common.Com_HashKey = (int (*)(char*, int))func_Com_HashKey_addr;
	proxy.server.common.Com_Printf = (void (QDECL *)(const char*, ...))func_Com_Printf_addr;
	proxy.server.common.Cvar_VariableString = (char* (*)(const char*))func_Cvar_VariableString_addr;
	proxy.server.common.FS_FOpenFileWrite = (fileHandle_t (*)(const char*))func_FS_FOpenFileWrite_addr;
	proxy.server.common.FS_ForceFlush = (void (*)(fileHandle_t))func_FS_ForceFlush_addr;
	proxy.server.common.FS_Initialized = (qboolean(*)())func_FS_Initialized_addr;
	proxy.server.common.FS_Write = (int (*)(const void*, int, fileHandle_t))func_FS_Write_addr;
	proxy.server.common.Netchan_TransmitNextFragment = (void (*)(netchan_t*))func_Netchan_TransmitNextFragment_addr;
	proxy.server.common.NET_AdrToString = (const char* (*)(netadr_t))func_NET_AdrToString_addr;
	proxy.server.common.MSG_Init = (void (*)(msg_t*, byte*, int))func_MSG_Init_addr;
	proxy.server.common.MSG_ReadByte = (int (*)(msg_t*))func_MSG_ReadByte_addr;
	proxy.server.common.MSG_ReadDeltaUsercmdKey = (void (*)(msg_t*, int, usercmd_t*, usercmd_t*))func_MSG_ReadDeltaUsercmdKey_addr;
	proxy.server.common.MSG_WriteBigString = (void (*)(msg_t*, const char*))func_MSG_WriteBigString_addr;
	proxy.server.common.MSG_WriteByte = (void (*)(msg_t*, int))func_MSG_WriteByte_addr;
	proxy.server.common.MSG_WriteDeltaEntity = (void (*)(msg_t*, struct entityState_s*, struct entityState_s*, qboolean))func_MSG_WriteDeltaEntity_addr;
	proxy.server.common.MSG_WriteLong = (void (*)(msg_t*, int))func_MSG_WriteLong_addr;
	proxy.server.common.MSG_WriteShort = (void (*)(msg_t*, int))func_MSG_WriteShort_addr;
	proxy.server.common.rd_flush = *(void (**)(char*))var_common_rd_flush_addr;
	proxy.server.common.Sys_IsLANAddress = (qboolean(*)(netadr_t))func_Sys_IsLANAddress_addr;
	proxy.server.common.Sys_Print = (void (*)(const char*))func_Sys_Print_addr;
}

// Update value of packets and FPS
// From JK2MV (updated version by fau)
void Proxy_Server_CalcPacketsAndFPS(int clientNum, int* packets, int* fps)
{
	int		lastCmdTime;
	int		lastPacketIndex = 0;
	int		i;

	lastCmdTime = proxy.clientData[clientNum].cmdStats[proxy.clientData[clientNum].cmdIndex & (CMD_MASK - 1)].serverTime;

	for (i = 0; i < CMD_MASK; i++)
	{
		ucmdStat_t* stat = &proxy.clientData[clientNum].cmdStats[i];

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

void Proxy_Server_UpdateUcmdStats(int clientNum, usercmd_t* cmd, int packetIndex)
{
	// TODO
	// Clean le buffer apres avoir changer de map
	// Reset le timer des 10 seconds par la meme occasion ?
	proxy.clientData[clientNum].cmdIndex++;
	proxy.clientData[clientNum].cmdStats[proxy.clientData[clientNum].cmdIndex & (CMD_MASK - 1)].serverTime = cmd->serverTime;
	proxy.clientData[clientNum].cmdStats[proxy.clientData[clientNum].cmdIndex & (CMD_MASK - 1)].packetIndex = packetIndex;
}

void Proxy_Server_UpdateTimenudge(client_t* client, usercmd_t* cmd, int _Milliseconds)
{
	proxy.clientData[getClientNumFromAddr(client)].timenudgeData.delayCount++;
	proxy.clientData[getClientNumFromAddr(client)].timenudgeData.delaySum += cmd->serverTime - proxy.server.svs->time;
	proxy.clientData[getClientNumFromAddr(client)].timenudgeData.pingSum += client->ping;

	// Wait 1000 ms so we have enough data when we'll calcul an approximation of the timenudge
	if (_Milliseconds < proxy.clientData[getClientNumFromAddr(client)].timenudgeData.lastTimeTimeNudgeCalculation + 1000)
	{
		return;
	}

	// ((serverTime - sv.time) + ping -18 + (1000/sv_fps)) * -1
	proxy.clientData[getClientNumFromAddr(client)].timenudge =
		(
			(proxy.clientData[getClientNumFromAddr(client)].timenudgeData.delaySum / (float)proxy.clientData[getClientNumFromAddr(client)].timenudgeData.delayCount)
			+ (proxy.clientData[getClientNumFromAddr(client)].timenudgeData.pingSum / (float)proxy.clientData[getClientNumFromAddr(client)].timenudgeData.delayCount)
			// this magic number might be the instructions time until the calc
#if defined(_WIN32) && !defined(MINGW32)
			- 21
#else
			- 19
#endif
			+ (1000 / (float)proxy.server.cvars.sv_fps->integer)
		) * -1;

	proxy.clientData[getClientNumFromAddr(client)].timenudgeData.delayCount = 0;
	proxy.clientData[getClientNumFromAddr(client)].timenudgeData.delaySum = 0;
	proxy.clientData[getClientNumFromAddr(client)].timenudgeData.pingSum = 0;

	proxy.clientData[getClientNumFromAddr(client)].timenudgeData.lastTimeTimeNudgeCalculation = _Milliseconds;
}