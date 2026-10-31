#pragma once

// ==================================================
// INCLUDE
// ==================================================

#include "JKA_YBEProxy/Proxy.hpp"
#include "sdk/qcommon/q_shared.hpp"
#include "sdk/server/server.hpp"
#include "JKA_YBEProxy/Engine/Addresses.hpp"

// ==================================================
// STRUCT
// ==================================================

struct serverFunctions_t
{
	void		(*SV_ClientEnterWorld)							(client_t*, usercmd_t*);
	void		(*SV_ClientThink)								(client_t*, usercmd_t*);
	void		(*SV_DropClient)								(client_t*, const char*);
	void		(*SV_Netchan_Transmit)							(client_t*, msg_t*);
	//int		(*SV_RateMsec)									(client_t*, int);
	void		(*SV_UpdateServerCommandsToClient)				(client_t*, msg_t*);
	void		(*SV_FlushRedirect)								(char*);
	void		(*SV_ExecuteClientCommand)						(client_t*, const char*, qboolean);
	void		(*SV_GetChallenge)								(netadr_t);
	void		(*SV_DirectConnect)								(netadr_t);
	void		(*SVC_RemoteCommand)							(netadr_t, msg_t*);
	qboolean	(*SV_Netchan_Process)							(client_t*, msg_t*);
	void		(*SV_SendMessageToClient)						(msg_t*, client_t*);
	void		(*SVC_Status)									(netadr_t);
	void		(*SVC_Info)										(netadr_t);
	void		(*SV_ConnectionlessPacket)						(netadr_t, msg_t*);
	void		(*SV_ExecuteClientMessage)						(client_t*, msg_t*);
	void		(*SV_SendClientGameState)						(client_t*);
	void		(QDECL* SV_SendServerCommand)					(client_t*, const char*, ...);
	void		(*SV_UserinfoChanged)							(client_t*);
	void		(*SV_SendClientSnapshot)						(client_t*);
};

struct serverCvars_t
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
	cvar_t* sv_floodProtect;
	cvar_t* sv_allowDownload;
};

struct vars_t
{
	char**			rd_buffer;
	int*			rd_buffersize;
	void			(* *rd_flush) (char*);
	fileHandle_t*	logfile;
	int*			cmd_argc;
	char*			(*cmd_argv)[MAX_STRING_TOKENS]; // char* cmd_argv[MAX_STRING_TOKENS];
	char			(*cmd_tokenized)[BIG_INFO_STRING + MAX_STRING_TOKENS]; // char cmd_tokenized[BIG_INFO_STRING+MAX_STRING_TOKENS];
};

struct Cvars_t
{
	cvar_t* com_dedicated;
	cvar_t* com_sv_running;
	cvar_t* com_cl_running;
	cvar_t* com_logfile;
	cvar_t* com_developer;
	cvar_t* fs_gamedirvar;
};

struct Functions_t
{
	void			(QDECL* Com_DPrintf)							(const char*, ...);
	int				(*Com_HashKey)									(char*, int);
	void			(QDECL *Com_Printf)								(const char*, ...);
	void			(*Com_BeginRedirect)							(char*, int, void (*)(char*));
	void			(*Com_EndRedirect)								(void);
	void			(*Com_WriteConfigToFile)						(const char*);
	char*			(*Cvar_VariableString)							(const char*);
	float			(*Cvar_VariableValue)							(const char*);
	fileHandle_t	(*FS_FOpenFileWrite)							(const char*);
	void			(*FS_ForceFlush)								(fileHandle_t);
	qboolean		(*FS_Initialized)								();
	int				(*FS_Write)										(const void*, int, fileHandle_t);
	int				(*FS_FileIsInPAK)								(const char*, int*);
	const char*		(*FS_LoadedPakPureChecksums)					(void);
	const char*		(*FS_ReferencedPakNames)						(void);
	int				(*FS_SV_FOpenFileRead)							(const char*, fileHandle_t*);
	int				(*FS_Read)										(void*, int, fileHandle_t);
	void			(*FS_FCloseFile)								(fileHandle_t);
	void			(*Netchan_TransmitNextFragment)					(netchan_t*);
	const char*		(*NET_AdrToString)								(netadr_t);
	void			(QDECL* NET_OutOfBandPrint)						(netsrc_t, netadr_t, const char*, ...);
	qboolean		(*NET_CompareBaseAdr)							(netadr_t, netadr_t);
	void			(*MSG_Init)										(msg_t*, byte*, int);
	int				(*MSG_ReadByte)									(msg_t*);
	void			(*MSG_ReadDeltaUsercmdKey)						(msg_t*, int, usercmd_t*, usercmd_t*);
	int				(*MSG_ReadShort)								(msg_t*);
	int				(*MSG_ReadLong)									(msg_t*);
	char*			(*MSG_ReadString)								(msg_t*);
	char*			(*MSG_ReadStringLine)							(msg_t*);
	void			(*MSG_Bitstream)								(msg_t*);
	void			(*MSG_BeginReadingOOB)							(msg_t*);
	void			(*MSG_WriteBigString)							(msg_t*, const char*);
	void			(*MSG_WriteByte)								(msg_t*, int);
	void			(*MSG_WriteDeltaEntity)							(msg_t*, struct entityState_s*, struct entityState_s*, qboolean);
	void			(*MSG_WriteLong)								(msg_t*, int);
	void			(*MSG_WriteShort)								(msg_t*, int);
	void			(*MSG_WriteString)								(msg_t*, const char*);
	void			(*MSG_WriteData)								(msg_t*, const void*, int);
	qboolean		(*Sys_IsLANAddress)								(netadr_t);
	void			(*Sys_Print)									(const char*);
	char*			(*Cmd_Argv)                                     (int);
	void			(*Cmd_ExecuteString)							(const char*);
	void			(*Cmd_TokenizeString)							(const char*);
	void			(*Huff_Decompress)								(msg_t*, int);
	void			(*Z_Free)										(void*);
	void*			(*Z_Malloc)										(int, memtag_t, qboolean, int);
	void			(*Z_TagFree)									(memtag_t);
};

struct Common_t
{
	vars_t 		vars;
	Cvars_t 	cvars;
	Functions_t	functions;
};

struct ProxyServer_t
{
	serverStatic_t*		svs;
	server_t*			sv;
	serverFunctions_t	functions;
	serverCvars_t		cvars;
	Common_t			common;
};

// ==================================================
// FUNCTION
// ==================================================

void Proxy_Engine_Initialize_MemoryLayer(void);

// ==================================================
// EXTERN VARIABLE
// ==================================================

// /!\ Should only be used when original jampded is used
extern ProxyServer_t server;

// The client number of a client of svs.clients
inline std::size_t getClientNumFromAddr(client_t* client)
{
	return client - (*(client_t**)var_svsClients_addr[PROXY_PLATFORM]);
}
