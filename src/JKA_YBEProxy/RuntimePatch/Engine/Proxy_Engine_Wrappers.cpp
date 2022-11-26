#include "Proxy_Engine_Wrappers.hpp"
#include "JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Utils.hpp"
#include "Patches/qcommon/Proxy_common.hpp"

ProxyServer_t server = { 0 };

void Proxy_Engine_Initialize_MemoryLayer(void)
{
	proxy.trap->Print("----- Proxy: Initializing memory layer\n");

	// ----------- SERVER

	// vars
	server.svs = (decltype(server.svs))var_svs_addr[PROXY_PLATFORM];
	server.sv = (decltype(server.sv))var_sv_addr[PROXY_PLATFORM];

	// cvars
	server.cvars.sv_fps = *(cvar_t**)cvar_sv_fps_addr[PROXY_PLATFORM];
	server.cvars.sv_gametype = *(cvar_t**)cvar_sv_gametype_addr[PROXY_PLATFORM];
	server.cvars.sv_hostname = *(cvar_t**)cvar_sv_hostname_addr[PROXY_PLATFORM];
	server.cvars.sv_mapname = *(cvar_t**)cvar_sv_mapname_addr[PROXY_PLATFORM];
	server.cvars.sv_maxclients = *(cvar_t**)cvar_sv_maxclients_addr[PROXY_PLATFORM];
	server.cvars.sv_privateClients = *(cvar_t**)cvar_sv_privateClients_addr[PROXY_PLATFORM];
	server.cvars.sv_pure = *(cvar_t**)cvar_sv_pure_addr[PROXY_PLATFORM];
	server.cvars.sv_maxRate = *(cvar_t**)cvar_sv_maxRate_addr[PROXY_PLATFORM];
	server.cvars.sv_rconPassword = *(cvar_t**)cvar_sv_rconPassword_addr[PROXY_PLATFORM];
	server.cvars.sv_floodProtect = *(cvar_t**)cvar_sv_floodProtect_addr[PROXY_PLATFORM];
	server.cvars.sv_allowDownload = *(cvar_t**)cvar_sv_allowDownload_addr[PROXY_PLATFORM];

	// functions
	server.functions.SV_ClientEnterWorld = (decltype(server.functions.SV_ClientEnterWorld))func_SV_ClientEnterWorld_addr[PROXY_PLATFORM];
	server.functions.SV_ClientThink = (decltype(server.functions.SV_ClientThink))func_SV_ClientThink_addr[PROXY_PLATFORM];
	server.functions.SV_DropClient = (decltype(server.functions.SV_DropClient))func_SV_DropClient_addr[PROXY_PLATFORM];
	server.functions.SV_Netchan_Transmit = (decltype(server.functions.SV_Netchan_Transmit))func_SV_Netchan_Transmit_addr[PROXY_PLATFORM];
	//server.functions.SV_RateMsec = (decltype(server.functions.SV_RateMsec))func_SV_RateMsec_addr[PROXY_PLATFORM]; // we directly use our own function inside "Proxy_sv_snapshot()"
	server.functions.SV_UpdateServerCommandsToClient = (decltype(server.functions.SV_UpdateServerCommandsToClient))func_SV_UpdateServerCommandsToClient_addr[PROXY_PLATFORM];
	server.functions.SV_FlushRedirect = (decltype(server.functions.SV_FlushRedirect))func_SV_FlushRedirect_addr[PROXY_PLATFORM];
	server.functions.SV_ExecuteClientCommand = (decltype(server.functions.SV_ExecuteClientCommand))func_SV_ExecuteClientCommand_addr[PROXY_PLATFORM];
	server.functions.SV_GetChallenge = (decltype(server.functions.SV_GetChallenge))func_SV_GetChallenge_addr[PROXY_PLATFORM];
	server.functions.SV_DirectConnect = (decltype(server.functions.SV_DirectConnect))func_SV_DirectConnect_addr[PROXY_PLATFORM];
	server.functions.SVC_RemoteCommand = (decltype(server.functions.SVC_RemoteCommand))func_SVC_RemoteCommand_addr[PROXY_PLATFORM];
	server.functions.SV_Netchan_Process = (decltype(server.functions.SV_Netchan_Process))func_SV_Netchan_Process_addr[PROXY_PLATFORM];
	server.functions.SV_SendMessageToClient = (decltype(server.functions.SV_SendMessageToClient))func_SV_SendMessageToClient_addr[PROXY_PLATFORM];
	server.functions.SVC_Status = (decltype(server.functions.SVC_Status))func_SVC_Status_addr[PROXY_PLATFORM];
	server.functions.SVC_Info = (decltype(server.functions.SVC_Info))func_SVC_Info_addr[PROXY_PLATFORM];
	server.functions.SV_ConnectionlessPacket = (decltype(server.functions.SV_ConnectionlessPacket))func_SV_ConnectionlessPacket_addr[PROXY_PLATFORM];
	server.functions.SV_ExecuteClientMessage = (decltype(server.functions.SV_ExecuteClientMessage))func_SV_ExecuteClientMessage_addr[PROXY_PLATFORM];
	server.functions.SV_SendClientGameState = (decltype(server.functions.SV_SendClientGameState))func_SV_SendClientGameState_addr[PROXY_PLATFORM];
	server.functions.SV_SendServerCommand = (decltype(server.functions.SV_SendServerCommand))func_SV_SendServerCommand_addr[PROXY_PLATFORM];
	server.functions.SV_UserinfoChanged = (decltype(server.functions.SV_UserinfoChanged))func_SV_UserinfoChanged_addr[PROXY_PLATFORM];
	server.functions.SV_SendClientSnapshot = (decltype(server.functions.SV_SendClientSnapshot))func_SV_SendClientSnapshot_addr[PROXY_PLATFORM];

	// ----------- COMMON

	// vars
	server.common.vars.logfile = (decltype(server.common.vars.logfile))var_common_logfile_addr[PROXY_PLATFORM];
	server.common.vars.rd_buffer = (decltype(server.common.vars.rd_buffer))var_common_rd_buffer_addr[PROXY_PLATFORM];
	server.common.vars.rd_buffersize = (decltype(server.common.vars.rd_buffersize))var_common_rd_buffersize_addr[PROXY_PLATFORM];
	server.common.vars.rd_flush = (decltype(server.common.vars.rd_flush))var_common_rd_flush_addr[PROXY_PLATFORM];
	server.common.vars.cmd_argc = (decltype(server.common.vars.cmd_argc))var_cmd_argc_addr[PROXY_PLATFORM];
	server.common.vars.cmd_argv = (decltype(server.common.vars.cmd_argv))var_cmd_argv_addr[PROXY_PLATFORM];
	server.common.vars.cmd_tokenized = (decltype(server.common.vars.cmd_tokenized))var_cmd_tokenized_addr[PROXY_PLATFORM];

	// cvars
	server.common.cvars.com_dedicated = *(cvar_t**)cvar_common_com_dedicated_addr[PROXY_PLATFORM];
	server.common.cvars.com_sv_running = *(cvar_t**)cvar_common_com_sv_running_addr[PROXY_PLATFORM];
	server.common.cvars.com_cl_running = *(cvar_t**)cvar_common_com_cl_running_addr[PROXY_PLATFORM];
	server.common.cvars.com_logfile = *(cvar_t**)cvar_common_com_logfile_addr[PROXY_PLATFORM];
	server.common.cvars.com_developer = *(cvar_t**)cvar_common_com_developer_addr[PROXY_PLATFORM];
	server.common.cvars.fs_gamedirvar = *(cvar_t**)cvar_common_fs_gamedirvar_addr[PROXY_PLATFORM];

	// functions
	server.common.functions.Com_DPrintf = (decltype(server.common.functions.Com_DPrintf))func_Com_DPrintf_addr[PROXY_PLATFORM];
	server.common.functions.Com_HashKey = (decltype(server.common.functions.Com_HashKey))func_Com_HashKey_addr[PROXY_PLATFORM];
	server.common.functions.Com_Printf = (decltype(server.common.functions.Com_Printf))func_Com_Printf_addr[PROXY_PLATFORM]; // It will call the fixed one in Proxy_files
	server.common.functions.Com_BeginRedirect = (decltype(server.common.functions.Com_BeginRedirect))func_Com_BeginRedirect_addr[PROXY_PLATFORM];
	
	// There's no function on Windows version as it was optimized, so do a proxy function
	// Use the define on Windows as the function address won't be accessible at compile time
	if (PROXY_PLATFORM == Proxy_Platform_e::PROXY_WINDOWS)
	{
		server.common.functions.Com_EndRedirect = (decltype(server.common.functions.Com_EndRedirect))&Proxy_Windows_Com_EndRedirect;
	}
	else
	{
		server.common.functions.Com_EndRedirect = (decltype(server.common.functions.Com_EndRedirect))func_Com_EndRedirect_addr[PROXY_PLATFORM];
	}

	server.common.functions.Cvar_VariableString = (decltype(server.common.functions.Cvar_VariableString))func_Cvar_VariableString_addr[PROXY_PLATFORM];
	server.common.functions.Cvar_VariableValue = (decltype(server.common.functions.Cvar_VariableValue))func_Cvar_VariableValue_addr[PROXY_PLATFORM];
	server.common.functions.FS_FOpenFileWrite = (decltype(server.common.functions.FS_FOpenFileWrite))func_FS_FOpenFileWrite_addr[PROXY_PLATFORM];
	server.common.functions.FS_ForceFlush = (decltype(server.common.functions.FS_ForceFlush))func_FS_ForceFlush_addr[PROXY_PLATFORM];
	server.common.functions.FS_Initialized = (decltype(server.common.functions.FS_Initialized))func_FS_Initialized_addr[PROXY_PLATFORM];
	server.common.functions.FS_Write = (decltype(server.common.functions.FS_Write))func_FS_Write_addr[PROXY_PLATFORM];
	server.common.functions.FS_FileIsInPAK = (decltype(server.common.functions.FS_FileIsInPAK))func_FS_FileIsInPAK_addr[PROXY_PLATFORM];
	server.common.functions.FS_LoadedPakPureChecksums = (decltype(server.common.functions.FS_LoadedPakPureChecksums))func_FS_LoadedPakPureChecksums_addr[PROXY_PLATFORM];
	server.common.functions.FS_ReferencedPakNames = (decltype(server.common.functions.FS_ReferencedPakNames))func_FS_ReferencedPakNames_addr[PROXY_PLATFORM];
	server.common.functions.FS_SV_FOpenFileRead = (decltype(server.common.functions.FS_SV_FOpenFileRead))func_FS_SV_FOpenFileRead_addr[PROXY_PLATFORM];
	server.common.functions.FS_Read = (decltype(server.common.functions.FS_Read))func_FS_Read_addr[PROXY_PLATFORM];
	server.common.functions.FS_FCloseFile = (decltype(server.common.functions.FS_FCloseFile))func_FS_FCloseFile_addr[PROXY_PLATFORM];
	server.common.functions.Netchan_TransmitNextFragment = (decltype(server.common.functions.Netchan_TransmitNextFragment))func_Netchan_TransmitNextFragment_addr[PROXY_PLATFORM];
	server.common.functions.NET_AdrToString = (decltype(server.common.functions.NET_AdrToString))func_NET_AdrToString_addr[PROXY_PLATFORM];
	server.common.functions.NET_OutOfBandPrint = (decltype(server.common.functions.NET_OutOfBandPrint))func_NET_OutOfBandPrint_addr[PROXY_PLATFORM];
	server.common.functions.NET_CompareBaseAdr = (decltype(server.common.functions.NET_CompareBaseAdr))func_NET_CompareBaseAdr_addr[PROXY_PLATFORM];
	server.common.functions.MSG_Init = (decltype(server.common.functions.MSG_Init))func_MSG_Init_addr[PROXY_PLATFORM];
	server.common.functions.MSG_ReadByte = (decltype(server.common.functions.MSG_ReadByte))func_MSG_ReadByte_addr[PROXY_PLATFORM];
	server.common.functions.MSG_ReadDeltaUsercmdKey = (decltype(server.common.functions.MSG_ReadDeltaUsercmdKey))func_MSG_ReadDeltaUsercmdKey_addr[PROXY_PLATFORM];
	server.common.functions.MSG_ReadShort = (decltype(server.common.functions.MSG_ReadShort))func_MSG_ReadShort_addr[PROXY_PLATFORM];
	server.common.functions.MSG_ReadLong = (decltype(server.common.functions.MSG_ReadLong))func_MSG_ReadLong_addr[PROXY_PLATFORM];
	server.common.functions.MSG_ReadString = (decltype(server.common.functions.MSG_ReadString))func_MSG_ReadString_addr[PROXY_PLATFORM];
	server.common.functions.MSG_ReadStringLine = (decltype(server.common.functions.MSG_ReadStringLine))func_MSG_ReadStringLine_addr[PROXY_PLATFORM];
	server.common.functions.MSG_Bitstream = (decltype(server.common.functions.MSG_Bitstream))func_MSG_Bitstream_addr[PROXY_PLATFORM];
	server.common.functions.MSG_BeginReadingOOB = (decltype(server.common.functions.MSG_BeginReadingOOB))func_MSG_BeginReadingOOB_addr[PROXY_PLATFORM];
	server.common.functions.MSG_WriteBigString = (decltype(server.common.functions.MSG_WriteBigString))func_MSG_WriteBigString_addr[PROXY_PLATFORM];
	server.common.functions.MSG_WriteByte = (decltype(server.common.functions.MSG_WriteByte))func_MSG_WriteByte_addr[PROXY_PLATFORM];
	server.common.functions.MSG_WriteDeltaEntity = (decltype(server.common.functions.MSG_WriteDeltaEntity))func_MSG_WriteDeltaEntity_addr[PROXY_PLATFORM];
	server.common.functions.MSG_WriteLong = (decltype(server.common.functions.MSG_WriteLong))func_MSG_WriteLong_addr[PROXY_PLATFORM];
	server.common.functions.MSG_WriteShort = (decltype(server.common.functions.MSG_WriteShort))func_MSG_WriteShort_addr[PROXY_PLATFORM];
	server.common.functions.MSG_WriteString = (decltype(server.common.functions.MSG_WriteString))func_MSG_WriteString_addr[PROXY_PLATFORM];
	server.common.functions.MSG_WriteData = (decltype(server.common.functions.MSG_WriteData))func_MSG_WriteData_addr[PROXY_PLATFORM];
	server.common.functions.Sys_IsLANAddress = (decltype(server.common.functions.Sys_IsLANAddress))func_Sys_IsLANAddress_addr[PROXY_PLATFORM];
	server.common.functions.Sys_Print = (decltype(server.common.functions.Sys_Print))func_Sys_Print_addr[PROXY_PLATFORM];
	server.common.functions.Cmd_Argv = (decltype(server.common.functions.Cmd_Argv))func_Cmd_Argv_addr[PROXY_PLATFORM];
	server.common.functions.Cmd_ExecuteString = (decltype(server.common.functions.Cmd_ExecuteString))func_Cmd_ExecuteString_addr[PROXY_PLATFORM];
	server.common.functions.Cmd_TokenizeString = (decltype(server.common.functions.Cmd_TokenizeString))func_Cmd_TokenizeString_addr[PROXY_PLATFORM];
	server.common.functions.Huff_Decompress = (decltype(server.common.functions.Huff_Decompress))func_Huff_Decompress_addr[PROXY_PLATFORM];
	server.common.functions.Z_Free = (decltype(server.common.functions.Z_Free))func_Z_Free_addr[PROXY_PLATFORM];
	server.common.functions.Z_Malloc = (decltype(server.common.functions.Z_Malloc))func_Z_Malloc_addr[PROXY_PLATFORM];

	proxy.trap->Print("----- Proxy: Memory layer properly initialized\n");
}