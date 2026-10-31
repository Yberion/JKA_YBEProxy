#include "JKA_YBEProxy/Features/Chat.hpp"
#include "JKA_YBEProxy/Proxy.hpp"

// Below the team gametypes the game turns say_team into say, so a spectator could not talk to the other spectators
// only. While the game runs the say_team of a spectator there, its chat only goes to the spectators, as a team chat.
// The say_team of a player stays public
static bool spectatorChat = false;

// The team of the client in its configstring (the game's sessionTeam). The playerState of a spectator who follows a
// player is a copy of that player's, team included
static int Proxy_ClientTeam(int clientNum)
{
	char info[MAX_INFO_STRING];

	proxy.trap->GetConfigstring(CS_PLAYERS + clientNum, info, sizeof(info));

	return atoi(Info_ValueForKey(info, "t"));
}

void Proxy_Chat_ClientCommand(int clientNum, const char* cmd)
{
	if (!Q_stricmp(cmd, "say_team") && proxy.cvars.sv_gametype.integer < GT_TEAM && Proxy_ClientTeam(clientNum) == TEAM_SPECTATOR)
	{
		spectatorChat = true;
	}
}

void Proxy_Chat_ClientCommandDone(void)
{
	spectatorChat = false;
}

const char* Proxy_SharedAPI_TeamChat(int clientNum, const char* text)
{
	// A broadcast cannot be split by team
	if (!spectatorChat || clientNum < 0 || std::strncmp(text, "chat \"", 6))
	{
		return text;
	}

	if (Proxy_ClientTeam(clientNum) != TEAM_SPECTATOR)
	{
		return nullptr;
	}

	// The game's say: "chat \"<name>^7" EC ": ^2<message>\"". A name cannot hold EC, so the first one ends it. Another
	// format (a mod) only gets the filter above
	const char* const name = text + 6;
	const char* const separator = std::strchr(name, EC[0]);

	if (!separator || separator - name < 2 || std::strncmp(separator - 2, "^7" EC ": ^2", 7))
	{
		return text;
	}

	// The game's say_team of the team gametypes: "tchat \"" EC "(<name>^7" EC ")" EC ": ^5<message>\""
	static char teamText[MAX_STRING_CHARS];

	Com_sprintf(teamText, sizeof(teamText), "tchat \"" EC "(%.*s^7" EC ")" EC ": " S_COLOR_CYAN "%s", (int)(separator - 2 - name), name,
		separator + 5);

	return teamText;
}
