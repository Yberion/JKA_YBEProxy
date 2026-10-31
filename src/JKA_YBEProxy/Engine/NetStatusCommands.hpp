#pragma once

namespace ybeproxy::engine::netstatus_commands
{

void NetStatus(int clientNum);
void ExplainNetStatus(int clientNum);
// The client commands of /netStatus. true: the command was one of them, the game must not get it
bool Run(int clientNum, const char* cmd);

} // namespace ybeproxy::engine::netstatus_commands
