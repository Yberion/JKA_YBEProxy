#pragma once

namespace ybeproxy::features::client_commands
{

// The checks of a client command before the game gets it (crash and exploit fixes, mute, anti-cheat). false: the
// command is dropped, and the client may have been dropped too
bool Allowed(int clientNum, const char* cmd);
// The client commands of the proxy (/myratio). true: the command was the proxy's, the game must not get it
bool Run(int clientNum, const char* cmd);
void MyRatio(int clientNum);
// Arguments that the game misreads, replaced for the game only (vote, callvote kick). Called once the command passed the
// checks, before the game runs it, and once it has run
void ReplaceArguments(const char* cmd);
void ClientCommandDone(void);
// The argument n of the command the game runs (G_ARGV), when the proxy replaces it: copied into buffer, returns true
bool GameArgv(int n, char* buffer, int bufferLength);

} // namespace ybeproxy::features::client_commands
