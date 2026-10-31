#pragma once

#include "sdk/game/g_public.hpp"

namespace ybeproxy::module::game_exports
{

// The commands of the game module (gameExport_t) that the proxy handles before the original game, one per vmMain
// command. They only call the features

// GAME_INIT: before the original game runs it, and after
void Init(void);
void InitDone(void);
// GAME_SHUTDOWN: before the original game runs it
void Shutdown(void);
void RunFrame(int levelTime);
const char* ClientConnect(int clientNum, qboolean firstTime, qboolean isBot);
// Called when the game accepted the client
void ClientConnected(int clientNum, qboolean isBot);
void ClientDisconnect(int clientNum);
void ClientBegin(int clientNum, qboolean allowTeamReset);
qboolean ClientCommand(int clientNum);
// Called once the game has run the client command
void ClientCommandDone(void);
// Returns qtrue when the command is the proxy's ("mute", "unmute", "proxyconfig"...), qfalse to let the game handle it
qboolean ConsoleCommand(void);
qboolean ClientUserinfoChanged(int clientNum);

/*
// If we want to do something with the usercmd then call manually GetUsercmd
void ClientThink(int clientNum, usercmd_t* ucmd);
*/

} // namespace ybeproxy::module::game_exports
