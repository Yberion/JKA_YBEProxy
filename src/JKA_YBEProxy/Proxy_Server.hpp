#pragma once

// ==================================================
// INCLUDE
// ==================================================

#include "game/g_local.hpp"
#include "server/server.hpp"

// ==================================================
// DEFINE
// ==================================================

#if defined(_WIN32) && !defined(MINGW32)
	// Function address to hook
	#define func_Com_Printf_addr 0x40fbe0
	#define func_SV_CalcPings_addr 0x444220
	#define func_SV_SendMessageToClient_addr 0x444dc0
	#define func_SV_UserMove_addr 0x43c210
	#define func_SV_SendClientGameState_addr 0x43ae70
	#define func_SV_Status_f_addr 0x43a4a0
	#define func_SV_SvEntityForGentity_addr 0x43cd80
    #define func_SVC_Status_addr 0x443870
	#define func_SVC_Info_addr 0x443a10
	#define func_SVC_RemoteCommand_addr 0x443ce0
	#define func_Cmd_TokenizeString_addr 0x40f580
	#define func_SV_UserinfoChanged_addr 0x43b8c0

	// Function address to call
	#define func_SV_ClientEnterWorld_addr 0x43b230
	#define func_SV_ClientThink_addr 0x43bba0
	#define func_SV_DropClient_addr 0x43bbf0
	#define func_SV_Netchan_Transmit_addr 0x444950
	//#define func_SV_RateMsec_addr 0x444d60
	#define func_SV_FlushRedirect_addr 0x443cb0
	#define func_SV_UpdateServerCommandsToClient_addr 0x444c80
	#define func_Com_DPrintf_addr 0x40fdb0
	#define func_Com_HashKey_addr 0x410370
	#define func_Com_BeginRedirect_addr 0x40fb70
	#define func_Com_EndRedirect_addr Windows_Com_EndRedirect // There's no function on Windows version as it was optimized, so do a proxy function
	#define func_Cvar_VariableString_addr 0x411ef0
	#define func_FS_FOpenFileWrite_addr 0x413690
	#define func_FS_ForceFlush_addr 0x413240
	#define func_FS_Initialized_addr 0x412e10
	#define func_FS_Write_addr 0x414350
	#define func_Netchan_TransmitNextFragment_addr 0x41a3b0
	#define func_NET_AdrToString_addr 0x419f10
	#define func_NET_OutOfBandPrint_addr 0x41a230
	#define func_MSG_Init_addr 0x419d00
	#define func_MSG_ReadByte_addr 0x4189f0
	#define func_MSG_ReadDeltaUsercmdKey_addr 0x418b50
	#define func_MSG_WriteBigString_addr 0x418940
	#define func_MSG_WriteByte_addr 0x418810
	#define func_MSG_WriteDeltaEntity_addr 0x418e40
	#define func_MSG_WriteLong_addr 0x418880
	#define func_MSG_WriteShort_addr 0x418860
	#define func_Sys_IsLANAddress_addr 0x457490
	#define func_Sys_Print_addr 0x44b930 // directly Conbuf_AppendText()
	#define func_Cmd_Argv_addr 0x40f490
	#define func_Cmd_ExecuteString_addr 0x40fa10

	// Function address called in ASM
	#define func_Sys_Milliseconds_addr 0x4580E0

	// Variable address
	#define var_svs_addr 0x606218
	#define var_svsClients_addr 0x606224
	#define var_sv_addr 0x567F30
	#define var_common_rd_buffer_addr 0x4e3790
	#define var_common_rd_buffersize_addr 0x4dc5ac
	#define var_common_rd_flush_addr 0x4dc73c
	#define var_common_logfile_addr 0x4e376c
	#define var_cmd_argc_addr 0x4dc188
	#define var_cmd_argv_addr 0x4d8d88
	#define var_cmd_tokenized_addr 0x4d9d88

	// cvar (for addr check in function: 0x442f60)
	#define cvar_sv_fps_addr 0x6102d0
	#define cvar_sv_gametype_addr 0x610298
	#define cvar_sv_hostname_addr 0x61027c
	#define cvar_sv_mapname_addr 0x61029c
	#define cvar_sv_maxclients_addr 0x610278
	#define cvar_sv_privateClients_addr 0x610274
	#define cvar_sv_pure_addr 0x60620c
	#define cvar_sv_maxRate_addr 0x6102b4
	#define cvar_sv_rconPassword_addr 0x606210
	#define cvar_common_com_dedicated_addr 0x4dc5dc
	#define cvar_common_com_sv_running_addr 0x4dc5e0
	#define cvar_common_com_logfile_addr 0x4dc5b8
	#define cvar_common_fs_gamedirvar_addr 0x4ff464

#else
	// Function address to hook
	#define func_Com_Printf_addr 0x8072ca4
	#define func_SV_CalcPings_addr 0x8057204
	#define func_SV_SendMessageToClient_addr 0x8058c84
	#define func_SV_UserMove_addr 0x804e6c4
	#define func_SV_SendClientGameState_addr 0x804cee4
	#define func_SV_Status_f_addr 0x804f7f4
	#define func_SV_SvEntityForGentity_addr 0x804ffb4
	#define func_SVC_Status_addr 0x8056574
	#define func_SVC_Info_addr 0x8056784
	#define func_SVC_RemoteCommand_addr 0x8056b14
	#define func_Cmd_TokenizeString_addr 0x812c454
	#define func_SV_UserinfoChanged_addr 0x804e144

	// Function address to call
	#define func_SV_ClientEnterWorld_addr 0x804d444
	#define func_SV_ClientThink_addr 0x804e634
	#define func_SV_DropClient_addr 0x804cb84
	#define func_SV_Netchan_Transmit_addr 0x8057db4
	//#define func_SV_RateMsec_addr 0x8058c04
	#define func_SV_FlushRedirect_addr 0x8057b44
	#define func_SV_UpdateServerCommandsToClient_addr 0x80582c4
	#define func_Com_DPrintf_addr 0x8072ed4
	#define func_Com_HashKey_addr 0x8073b14
	#define func_Com_BeginRedirect_addr 0x8072c34
	#define func_Com_EndRedirect_addr 0x8072c74
	#define func_Cvar_VariableString_addr 0x80756f4
	#define func_FS_FOpenFileWrite_addr 0x812d2a4
	#define func_FS_ForceFlush_addr 0x812c8a4
	#define func_FS_Initialized_addr 0x812b754
	#define func_FS_Write_addr 0x812e074
	#define func_Netchan_TransmitNextFragment_addr 0x807ab74
	#define func_NET_AdrToString_addr 0x807b314
	#define func_NET_OutOfBandPrint_addr 0x807b744 // won't be used on Linux for now
	#define func_MSG_Init_addr 0x80774a4
	#define func_MSG_ReadByte_addr 0x8077df4
	#define func_MSG_ReadDeltaUsercmdKey_addr 0x8078b34
	#define func_MSG_WriteBigString_addr 0x8077c04
	#define func_MSG_WriteByte_addr 0x8077a24
	#define func_MSG_WriteDeltaEntity_addr 0x8078d74
	#define func_MSG_WriteLong_addr 0x8077ad4
	#define func_MSG_WriteShort_addr 0x8077aa4
	#define func_Sys_IsLANAddress_addr 0x80c5f84
	#define func_Sys_Print_addr 0x80c57a4
	#define func_Cmd_Argv_addr 0x812c264
	#define func_Cmd_ExecuteString_addr 0x8124144

	// Function address called in ASM
	#define func_Sys_Milliseconds_addr 0x80c6714

	// Variable address
	#define var_svs_addr 0x83121e0
	#define var_svsClients_addr 0x83121ec
	#define var_sv_addr 0x8273ec0
	#define var_common_rd_buffer_addr 0x81e90c0
	#define var_common_rd_buffersize_addr 0x81e90c4
	#define var_common_rd_flush_addr 0x81e90c8
	#define var_common_logfile_addr 0x831f24c
	#define var_cmd_argc_addr 0x8260e20
	#define var_cmd_argv_addr 0x8260e40
	#define var_cmd_tokenized_addr 0x8264440

	// cvar (for addr check in function: 0x8055824)
	#define cvar_sv_fps_addr 0x8273e84
	#define cvar_sv_gametype_addr 0x83121cc
	#define cvar_sv_hostname_addr 0x8273e9c
	#define cvar_sv_mapname_addr 0x8273e90
	#define cvar_sv_maxclients_addr 0x8273ea4
	#define cvar_sv_privateClients_addr 0x8273e80
	#define cvar_sv_pure_addr 0x83121a8
	#define cvar_sv_maxRate_addr 0x831219c
	#define cvar_sv_rconPassword_addr 0x83121d4
	#define cvar_common_com_dedicated_addr 0x831f254
	#define cvar_common_com_sv_running_addr 0x831f300
	#define cvar_common_com_logfile_addr 0x831f41c
	#define cvar_common_fs_gamedirvar_addr 0x838aaec
	
#endif

#define getClientNumFromAddr(client_ptr) ((client_ptr) - (*(client_t **)var_svsClients_addr))

// ==================================================
// STRUCT
// ==================================================

typedef struct serverFunctions_s
{
	void		(*SV_ClientEnterWorld)							(client_t*, usercmd_t*);
	void		(*SV_ClientThink)								(client_t*, usercmd_t*);
	void		(*SV_DropClient)								(client_t*, const char*);
	void		(*SV_Netchan_Transmit)							(client_t*, msg_t*);
	//int		(*SV_RateMsec)									(client_t*, int);
	void		(*SV_UpdateServerCommandsToClient)				(client_t*, msg_t*);
	void		(*SV_FlushRedirect)								(char* outputbuf);
} serverFunctions_t;

typedef struct serverCvars_s
{
	cvar_t* sv_fps;
	cvar_t* sv_gametype;
	cvar_t* sv_hostname;
	cvar_t* sv_mapname;
	cvar_t* sv_maxclients;
	cvar_t* sv_privateClients;
	cvar_t* sv_pure;
	cvar_t* sv_maxRate;
	cvar_t* sv_rconPassword;
} serverCvars_t;

typedef struct Common_s
{
	struct vars_s
	{
		char**			rd_buffer;
		int*			rd_buffersize;
		void			(* *rd_flush) (char*);
		fileHandle_t*	logfile;
		int*			cmd_argc;
		char*			(*cmd_argv)[MAX_STRING_TOKENS]; // char* cmd_argv[MAX_STRING_TOKENS];
		char			(*cmd_tokenized)[BIG_INFO_STRING + MAX_STRING_TOKENS]; // char cmd_tokenized[BIG_INFO_STRING+MAX_STRING_TOKENS];
	} vars;

	struct Cvars_s
	{
		cvar_t* com_dedicated;
		cvar_t* com_sv_running;
		cvar_t* com_logfile;
		cvar_t* fs_gamedirvar;
	} cvars;

	struct Functions_s
	{
		void			(QDECL* Com_DPrintf)							(const char*, ...);
		int				(*Com_HashKey)									(char*, int);
		void			(QDECL *Com_Printf)								(const char*, ...);
		void			(*Com_BeginRedirect)							(char*, int, void (*)(char*));
		void			(*Com_EndRedirect)								(void);
		char*			(*Cvar_VariableString)							(const char*);
		fileHandle_t	(*FS_FOpenFileWrite)							(const char*);
		void			(*FS_ForceFlush)								(fileHandle_t);
		qboolean		(*FS_Initialized)								();
		int				(*FS_Write)										(const void*, int, fileHandle_t);
		void			(*Netchan_TransmitNextFragment)					(netchan_t*);
		const char*		(*NET_AdrToString)								(netadr_t);
		void			(QDECL* NET_OutOfBandPrint)						(netsrc_t, netadr_t, const char*, ...);
		void			(*MSG_Init)										(msg_t*, byte*, int);
		int				(*MSG_ReadByte)									(msg_t*);
		void			(*MSG_ReadDeltaUsercmdKey)						(msg_t*, int, usercmd_t*, usercmd_t*);
		void			(*MSG_WriteBigString)							(msg_t*, const char*);
		void			(*MSG_WriteByte)								(msg_t*, int);
		void			(*MSG_WriteDeltaEntity)							(msg_t*, struct entityState_s*, struct entityState_s*, qboolean);
		void			(*MSG_WriteLong)								(msg_t*, int);
		void			(*MSG_WriteShort)								(msg_t*, int);
		qboolean		(*Sys_IsLANAddress)								(netadr_t);
		void			(*Sys_Print)									(const char*);
		char*			(*Cmd_Argv)                                     (int arg);
		void			(*Cmd_ExecuteString)							(const char* text);
	} functions;
} common_t;

typedef struct ProxyServer_s
{
	serverStatic_t*		svs;
	server_t*			sv;
	serverFunctions_t	functions;
	serverCvars_t		cvars;
	common_t			common;
} ProxyServer_t;

// ==================================================
// EXTERN VARIABLE
// ==================================================

// /!\ Should only be used when original jampded is used
extern ProxyServer_t server;