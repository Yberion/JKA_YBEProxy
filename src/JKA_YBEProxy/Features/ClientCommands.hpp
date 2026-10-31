#pragma once

// The checks of a client command before the game gets it (crash and exploit fixes, mute, anti-cheat). false: the
// command is dropped, and the client may have been dropped too
bool Proxy_ClientCommands_Allowed(int clientNum, const char* cmd);
// The client commands of the proxy (/myratio). true: the command was the proxy's, the game must not get it
bool Proxy_ClientCommands_Run(int clientNum, const char* cmd);
void Proxy_ClientCommand_MyRatio(int clientNum);
