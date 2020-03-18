#include "Proxy_Server.h"
#include "Proxy_Header.h"

void Proxy_Server_Initialize_MemoryAddress(void)
{
	proxy.server.svs = (serverStatic_t*)var_svs_addr;
	proxy.server.sv = (server_t*)var_sv_addr;

	proxy.server.cvar.sv_sv_maxclients = *(cvar_t**)cvar_sv_maxclients_addr;
	proxy.server.cvar.sv_fps = *(cvar_t**)cvar_sv_fps_addr; 
	proxy.server.cvar.sv_pure = *(cvar_t**)cvar_sv_pure_addr;

	proxy.server.functions.SV_ClientEnterWorld = (void (*)(client_t*, usercmd_t*))func_SV_ClientEnterWorld_addr;
	proxy.server.functions.SV_ClientThink = (void (*)(client_t*, usercmd_t*))func_SV_ClientThink_addr;
	proxy.server.functions.SV_DropClient = (void (*)(client_t*, const char*))func_SV_DropClient_addr;
	proxy.server.functions.SV_Netchan_Transmit = (void (*)(client_t*, msg_t*))func_SV_Netchan_Transmit_addr;
	proxy.server.functions.SV_RateMsec = (int (*)(client_t*, int))func_SV_RateMsec_addr;
	proxy.server.functions.Com_HashKey = (int (*)(char*, int))func_Com_HashKey_addr;
	proxy.server.functions.Com_Printf = (void (*)(const char*, ...))func_Com_Printf_addr;
	proxy.server.functions.Netchan_TransmitNextFragment = (void (*)(netchan_t *))func_Netchan_TransmitNextFragment_addr;
	proxy.server.functions.MSG_ReadByte = (int (*)(msg_t*))func_MSG_ReadByte_addr;
	proxy.server.functions.MSG_ReadDeltaUsercmdKey = (void (*)(msg_t*, int, usercmd_t*, usercmd_t*))func_MSG_ReadDeltaUsercmdKey_addr;
	proxy.server.functions.Sys_IsLANAddress = (qboolean (*)(netadr_t))func_Sys_IsLANAddress_addr;
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
			+ (1000 / (float)proxy.server.cvar.sv_fps->integer)
		) * -1;

	proxy.clientData[getClientNumFromAddr(client)].timenudgeData.delayCount = 0;
	proxy.clientData[getClientNumFromAddr(client)].timenudgeData.delaySum = 0;
	proxy.clientData[getClientNumFromAddr(client)].timenudgeData.pingSum = 0;

	proxy.clientData[getClientNumFromAddr(client)].timenudgeData.lastTimeTimeNudgeCalculation = _Milliseconds;
}