#pragma once

#include "sdk/game/g_public.hpp"

// The commands of the game module (gameExport_t) that the proxy handles before the original game, one per vmMain
// command. They only call the features

const char* Proxy_SharedAPI_ClientConnect(int clientNum, qboolean firstTime, qboolean isBot);
// Called when the game accepted the client
void Proxy_SharedAPI_ClientConnected(int clientNum, qboolean isBot);
void Proxy_SharedAPI_ClientDisconnect(int clientNum);
void Proxy_SharedAPI_ClientBegin(int clientNum, qboolean allowTeamReset);
qboolean Proxy_SharedAPI_ClientCommand(int clientNum);
// Called once the game has run the client command
void Proxy_SharedAPI_ClientCommandDone(void);
// Returns qtrue when the command is the proxy's ("mute", "unmute", "proxyconfig"...), qfalse to let the game handle it
qboolean Proxy_SharedAPI_ConsoleCommand(void);
qboolean Proxy_SharedAPI_ClientUserinfoChanged(int clientNum);

/*
// If we want to do something with the usercmd then call manually GetUsercmd
void Proxy_SharedAPI_ClientThink(int clientNum, usercmd_t* ucmd);
*/
