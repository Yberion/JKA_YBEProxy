#pragma once

// Called before the game runs a client command, and once it has run it
void Proxy_Chat_ClientCommand(int clientNum, const char* cmd);
void Proxy_Chat_ClientCommandDone(void);
// The text of a G_SEND_SERVER_COMMAND of the game: text itself, another text, or nullptr to drop it (say_team of a
// spectator below the team gametypes, see Chat.cpp)
const char* Proxy_SharedAPI_TeamChat(int clientNum, const char* text);
