#pragma once

namespace ybeproxy::features::client_commands
{

// The checks of a client command before the game gets it (crash and exploit fixes, mute, anti-cheat). false: the
// command is dropped, and the client may have been dropped too
bool Allowed(int clientNum, const char* cmd);
// The client commands of the proxy (/myratio). true: the command was the proxy's, the game must not get it
bool Run(int clientNum, const char* cmd);
void MyRatio(int clientNum);

} // namespace ybeproxy::features::client_commands
