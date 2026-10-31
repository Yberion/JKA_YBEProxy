#pragma once

// The addresses of the original engine, { Windows (jampDed.exe), Linux (linuxjampded) }: index them with PROXY_PLATFORM

#include <cstddef>
#include <cstdint>

// ==================================================
// DEFINE
// ==================================================

// constexpr intptr_t ADDRESS[] = { WINDOWS_ADDRESS, LINUX_ADDRESS };

// --------------------------------
// Address to NOPs
// --------------------------------

// We want to NOP mostly all bytes related to the timer check, including the call to Com_Milliseconds()
constexpr intptr_t func_SVC_RemoteCommand_timer_start_block_addr[] = { 0x443ced, 0x8056b26 };
// from this address, we want to NOP 25 bytes
constexpr std::size_t func_SVC_RemoteCommand_timer_NOP_amount[] = { 25, 25 };

// --------------------------------
// Address to patch a single byte
// --------------------------------

// the address to the byte that will allow use to change the value from 49136 to 1008
constexpr intptr_t func_SVC_RemoteCommand_Com_BeginRedirect_SV_OUTPUTBUF_LENGTH_addr[] = { 0x443de8, 0x8056c7d };

// Info_SetValueForKey and Info_SetValueForKey_Big of the engine test "length > MAX" (cmp, jbe), so a result of exactly
// MAX_INFO_STRING / BIG_INFO_STRING passes and its terminating NUL is written one byte past the buffer. The jbe (0x76)
// becomes jb (0x72), which is ">=" as in ioq3 and JK2MV
constexpr intptr_t func_Info_SetValueForKey_length_jbe_addr[] =		{ 0x41beb2, 0x807fe24 };
constexpr intptr_t func_Info_SetValueForKey_Big_length_jbe_addr[] =	{ 0x41c091, 0x8080024 };

// --------------------------------
// Address to patch several bytes
// --------------------------------

// SV_MapRestart_f sets "client->state = CS_ACTIVE" then calls SV_ClientEnterWorld for every client from CS_CONNECTED
// up. The mov becomes "cmp client->state, CS_ACTIVE", a jne past the call and a nop to fill:
// - Windows: mov dword ptr [esi], 4 (6 bytes), the jne goes to the "add esp, 8" that follows the call
// - Linux: mov dword ptr [esi + ecx], 4 (7 bytes), the jne goes to the instruction after the call
constexpr intptr_t func_SV_MapRestart_f_set_active_addr[] =				{ 0x439f1f, 0x804f23f };
constexpr unsigned char func_SV_MapRestart_f_set_active_bytes[][7] =	{ { 0x83, 0x3E, 0x04, 0x75, 0x06, 0x90 }, { 0x83, 0x3C, 0x0E, 0x04, 0x75, 0x13, 0x90 } };
constexpr std::size_t func_SV_MapRestart_f_set_active_length[] =		{ 6, 7 };

// --------------------------------
// Function address to hook
// --------------------------------

constexpr intptr_t func_Com_Printf_addr[] = 				{ 0x40fbe0, 0x8072ca4 };
constexpr intptr_t func_Com_WriteConfig_f_addr[] = 			{ 0x410680, 0x8074d64 };
constexpr intptr_t func_Info_Print_addr[] = 				{ 0x410ce0, 0x8073504 };
constexpr intptr_t func_SV_CalcPings_addr[] = 				{ 0x444220, 0x8057204 };
constexpr intptr_t func_SV_SendMessageToClient_addr[] = 	{ 0x444dc0, 0x8058c84 };
constexpr intptr_t func_SV_SendClientGameState_addr[] = 	{ 0x43ae70, 0x804cee4 };
constexpr intptr_t func_SV_DumpUser_f_addr[] =				{ 0x43a8c0, 0x804fd74 };
constexpr intptr_t func_SV_Status_f_addr[] = 				{ 0x43a4a0, 0x804f7f4 };
constexpr intptr_t func_SV_SvEntityForGentity_addr[] = 		{ 0x43cd80, 0x804ffb4 };
constexpr intptr_t func_SV_ConnectionlessPacket_addr[] = 	{ 0x443f00, 0x8056d64 };
constexpr intptr_t func_SVC_Status_addr[] = 				{ 0x443870, 0x8056574 };
constexpr intptr_t func_SVC_Info_addr[] = 					{ 0x443a10, 0x8056784 };
constexpr intptr_t func_SVC_RemoteCommand_addr[] = 			{ 0x443ce0, 0x8056b14 };
constexpr intptr_t func_Cmd_TokenizeString_addr[] = 		{ 0x40f580, 0x812c454 };
constexpr intptr_t func_SV_UserinfoChanged_addr[] = 		{ 0x43b8c0, 0x804e144 };
constexpr intptr_t func_SV_ExecuteClientMessage_addr[] = 	{ 0x43c3a0, 0x804e8c4 };
constexpr intptr_t func_SV_PacketEvent_addr[] = 			{ 0x4440e0, 0x8057024 };
constexpr intptr_t func_SV_ExecuteClientCommand_addr[] = 	{ 0x43bad0, 0x804e3d4 };
constexpr intptr_t func_SV_BeginDownload_f_addr[] = 		{ 0x43b3c0, 0x804d714 };
constexpr intptr_t func_SV_DoneDownload_f_addr[] = 			{ 0x43b390, 0x804eaa4 };
constexpr intptr_t func_SV_WriteDownloadToClient_addr[] =	{ 0x43b3f0, 0x804d764 };

// --------------------------------
// Function address to call
// --------------------------------

constexpr intptr_t func_SV_ClientEnterWorld_addr[] = 				{ 0x43b230, 0x804d444 };
constexpr intptr_t func_SV_ClientThink_addr[] = 					{ 0x43bba0, 0x804e634 };
constexpr intptr_t func_SV_DropClient_addr[] = 						{ 0x43bbf0, 0x804cb84 };
constexpr intptr_t func_SV_Netchan_Transmit_addr[] = 				{ 0x444950, 0x8057db4 };
//constexpr intptr_t func_SV_RateMsec_addr[] = 						{ 0x444d60, 0x8058c04 };
constexpr intptr_t func_SV_FlushRedirect_addr[] = 					{ 0x443cb0, 0x8057b44 };
constexpr intptr_t func_SV_UpdateServerCommandsToClient_addr[] =	{ 0x444c80, 0x80582c4 };
constexpr intptr_t func_SV_GetChallenge_addr[] = 					{ 0x43aa60, 0x804b9a4 };
constexpr intptr_t func_SV_DirectConnect_addr[] = 					{ 0x43c4e0, 0x804c014 };
constexpr intptr_t func_SV_Netchan_Process_addr[] = 				{ 0x444990, 0x8057e14 };
constexpr intptr_t func_SV_SendServerCommand_addr[] = 				{ 0x4435d0, 0x8056214 };
constexpr intptr_t func_SV_SendClientSnapshot_addr[] = 				{ 0x4455c0, 0x8058e04 };
constexpr intptr_t func_Com_DPrintf_addr[] = 						{ 0x40fdb0, 0x8072ed4 };
constexpr intptr_t func_Com_HashKey_addr[] = 						{ 0x410370, 0x8073b14 };
constexpr intptr_t func_Com_BeginRedirect_addr[] = 					{ 0x40fb70, 0x8072c34 };
constexpr intptr_t func_Com_WriteConfigToFile_addr[] = 				{ 0x410630, 0x8074cb4 };
// There's no function on Windows version as it was optimized, so do a proxy function
// Use the define on Windows as the function address won't be accessible at compile time
#define Proxy_Windows_Com_EndRedirect_ptr &Proxy_Windows_Com_EndRedirect
constexpr intptr_t func_Com_EndRedirect_addr[] = 					{ 0x000000, 0x8072c74};
constexpr intptr_t func_Cvar_VariableString_addr[] = 				{ 0x411ef0, 0x80756f4 };
constexpr intptr_t func_Cvar_VariableValue_addr[] = 				{ 0x411e70, 0x8075694 };
constexpr intptr_t func_FS_FOpenFileWrite_addr[] = 					{ 0x413690, 0x812d2a4 };
constexpr intptr_t func_FS_ForceFlush_addr[] = 						{ 0x413240, 0x812c8a4 };
constexpr intptr_t func_FS_Initialized_addr[] = 					{ 0x412e10, 0x812b754 };
constexpr intptr_t func_FS_Write_addr[] = 							{ 0x414350, 0x812e074 };
constexpr intptr_t func_FS_FileIsInPAK_addr[] = 					{ 0x414580, 0x812e354 };
constexpr intptr_t func_FS_LoadedPakPureChecksums_addr[] = 			{ 0x415de0, 0x8130d44 };
constexpr intptr_t func_FS_ReferencedPakNames_addr[] = 				{ 0x415ea0, 0x8131024 };
constexpr intptr_t func_FS_SV_FOpenFileRead_addr[] = 				{ 0x4133e0, 0x812cd74 };
constexpr intptr_t func_FS_Read_addr[] = 							{ 0x414280, 0x812df44 };
constexpr intptr_t func_FS_FCloseFile_addr[] = 						{ 0x4135e0, 0x812d1b4 };
constexpr intptr_t func_Netchan_TransmitNextFragment_addr[] = 		{ 0x41a3b0, 0x807ab74 };
constexpr intptr_t func_NET_AdrToString_addr[] = 					{ 0x419f10, 0x807b314 };
constexpr intptr_t func_NET_OutOfBandPrint_addr[] = 				{ 0x41a230, 0x807b744 };
constexpr intptr_t func_NET_CompareBaseAdr_addr[] = 				{ 0x419e90, 0x807b284 };
constexpr intptr_t func_MSG_Init_addr[] = 							{ 0x419d00, 0x80774a4 };
constexpr intptr_t func_MSG_ReadBits_addr[] = 						{ 0x4186d0, 0x8077834 };
constexpr intptr_t func_MSG_ReadByte_addr[] = 						{ 0x4189f0, 0x8077df4 };
constexpr intptr_t func_MSG_ReadDeltaUsercmdKey_addr[] = 			{ 0x418b50, 0x8078b34 };
constexpr intptr_t func_MSG_ReadShort_addr[] = 						{ 0x418a20, 0x8077e34 };
constexpr intptr_t func_MSG_ReadLong_addr[] = 						{ 0x418a50, 0x8077e74 };
constexpr intptr_t func_MSG_ReadString_addr[] = 					{ 0x418a70, 0x8077ee4 };
constexpr intptr_t func_MSG_ReadStringLine_addr[] = 				{ 0x418ae0, 0x8077ff4 };
constexpr intptr_t func_MSG_Bitstream_addr[] = 						{ 0x418500, 0x80775d4 };
constexpr intptr_t func_MSG_BeginReadingOOB_addr[] = 				{ 0x418510, 0x8077614 };
constexpr intptr_t func_MSG_WriteBigString_addr[] = 				{ 0x418940, 0x8077c04 };
constexpr intptr_t func_MSG_WriteByte_addr[] = 						{ 0x418810, 0x8077a24 };
constexpr intptr_t func_MSG_WriteDeltaEntity_addr[] = 				{ 0x418e40, 0x8078d74 };
constexpr intptr_t func_MSG_WriteLong_addr[] = 						{ 0x418880, 0x8077ad4 };
constexpr intptr_t func_MSG_WriteShort_addr[] = 					{ 0x418860, 0x8077aa4 };
constexpr intptr_t func_MSG_WriteString_addr[] = 					{ 0x4188a0, 0x8077b34 };
constexpr intptr_t func_MSG_WriteData_addr[] = 						{ 0x418530, 0x8077a54 };
constexpr intptr_t func_Sys_IsLANAddress_addr[] = 					{ 0x457490, 0x80c5f84 };
// Windows: directly Conbuf_AppendText()
constexpr intptr_t func_Sys_Print_addr[] = 							{ 0x44b930, 0x80c57a4 };
constexpr intptr_t func_Cmd_Argv_addr[] = 							{ 0x40f490, 0x812c264 };
constexpr intptr_t func_Cmd_ExecuteString_addr[] = 					{ 0x40fa10, 0x8124144 };
constexpr intptr_t func_Huff_Decompress_addr[] = 					{ 0x417900, 0x807c224 };
constexpr intptr_t func_Z_Free_addr[] = 							{ 0x426c40, 0x8080474 };
constexpr intptr_t func_Z_Malloc_addr[] = 							{ 0x427000, 0x80800f4 };
constexpr intptr_t func_Z_TagFree_addr[] = 							{ 0x426ca0, 0x80804e4 };

// --------------------------------
// Variable address
// --------------------------------

constexpr intptr_t var_svs_addr[] = 					{ 0x606218, 0x83121e0 };
constexpr intptr_t var_svsClients_addr[] = 				{ 0x606224, 0x83121ec };
constexpr intptr_t var_sv_addr[] = 						{ 0x567F30, 0x8273ec0 };
constexpr intptr_t var_common_rd_buffer_addr[] = 		{ 0x4e3790, 0x81e90c0 };
constexpr intptr_t var_common_rd_buffersize_addr[] = 	{ 0x4dc5ac, 0x81e90c4 };
constexpr intptr_t var_common_rd_flush_addr[] = 		{ 0x4dc73c, 0x81e90c8 };
constexpr intptr_t var_common_logfile_addr[] = 			{ 0x4e376c, 0x831f24c };
constexpr intptr_t var_cmd_argc_addr[] = 				{ 0x4dc188, 0x8260e20 };
constexpr intptr_t var_cmd_argv_addr[] = 				{ 0x4d8d88, 0x8260e40 };
constexpr intptr_t var_cmd_tokenized_addr[] = 			{ 0x4d9d88, 0x8264440 };

// --------------------------------
// Cvars
// for 'server' addr check function:
//   - Windows: 0x442f60
//   - Linux: 0x8055824
// 
// for 'com' addr check function:
//   - Windows: 0x410fc0
//   - Linux: 0x8074614
// --------------------------------

constexpr intptr_t cvar_sv_fps_addr[] = 				{ 0x6102d0, 0x8273e84 };
constexpr intptr_t cvar_sv_gametype_addr[] = 			{ 0x610298, 0x83121cc };
constexpr intptr_t cvar_sv_hostname_addr[] = 			{ 0x61027c, 0x8273e9c };
constexpr intptr_t cvar_sv_mapname_addr[] = 			{ 0x61029c, 0x8273e90 };
constexpr intptr_t cvar_sv_maxclients_addr[] = 			{ 0x610278, 0x8273ea4 };
constexpr intptr_t cvar_sv_privateClients_addr[] = 		{ 0x610274, 0x8273e80 };
constexpr intptr_t cvar_sv_pure_addr[] = 				{ 0x60620c, 0x83121a8 };
constexpr intptr_t cvar_sv_maxRate_addr[] = 			{ 0x6102b4, 0x831219c };
constexpr intptr_t cvar_sv_rconPassword_addr[] = 		{ 0x606210, 0x83121d4 };
constexpr intptr_t cvar_sv_floodProtect_addr[] = 		{ 0x610288, 0x8273e88 };
constexpr intptr_t cvar_sv_allowDownload_addr[] = 		{ 0x610290, 0x83121b0 };
constexpr intptr_t cvar_common_com_dedicated_addr[] = 	{ 0x4dc5dc, 0x831f254 };
constexpr intptr_t cvar_common_com_sv_running_addr[] =	{ 0x4dc5e0, 0x831f300 };
constexpr intptr_t cvar_common_com_cl_running_addr[] = 	{ 0x4e3778, 0x831f400 };
constexpr intptr_t cvar_common_com_logfile_addr[] = 	{ 0x4dc5b8, 0x831f41c };
constexpr intptr_t cvar_common_com_developer_addr[] = 	{ 0x4dc5a4, 0x831e204 };
constexpr intptr_t cvar_common_fs_gamedirvar_addr[] = 	{ 0x4ff464, 0x838aaec };
