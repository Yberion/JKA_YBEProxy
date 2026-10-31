#pragma once

void Proxy_Engine_ClientCommand_NetStatus(int clientNum);
void Proxy_Engine_ClientCommand_ExplainNetStatus(int clientNum);
// The client commands of /netStatus. true: the command was one of them, the game must not get it
bool Proxy_Engine_NetStatusCommands_Run(int clientNum, const char* cmd);
