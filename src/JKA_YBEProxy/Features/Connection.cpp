#include "JKA_YBEProxy/Features/Connection.hpp"
#include "JKA_YBEProxy/Proxy.hpp"
#include "JKA_YBEProxy/Features/Bans.hpp"

namespace ybeproxy::features::connection
{

// "1.2.3.4:29070" -> "1.2.3.4"
static void GetBaseAddress(const char* ip, char* out, std::size_t outSize)
{
	const char* port = std::strrchr(ip, ':');
	const std::size_t len = port ? (std::size_t)(port - ip) : std::strlen(ip);

	Q_strncpyz(out, ip, (outSize < len + 1) ? outSize : len + 1);
}

const char* Check(int clientNum, qboolean firstTime)
{
	// SV_DirectConnect adds the "ip" key that the game's ban check reads. When the userinfo was too long to receive it,
	// the key is missing, and a banned address would pass
	char userinfo[MAX_INFO_STRING];

	proxy.trap->GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	if (!Info_ValueForKey(userinfo, "ip")[0])
	{
		// Same message as ioq3, JK2MV and OpenJK give when the connection is refused for that reason
		return "Userinfo string length exceeded.  Try removing setu cvars from your config.";
	}

	// The ban list is the proxy's, the game's own is empty (see Bans.cpp). Same reason as the game's
	if (features::bans::IsBanned(Info_ValueForKey(userinfo, "ip")))
	{
		return "Banned.";
	}

	// Limit of clients per IP address, from Quake3e's sv_maxclientsPerIP (SV_DirectConnect): a new connection is refused
	// when the other clients of the address already reach the limit
	// Several players can share one address (LAN, carrier NAT): the limit is a setting. Only for a new connection: at
	// each map change the engine connects again the clients that are already in
	if (firstTime && proxy.cvars.proxy_sv_maxClientsPerIP.integer > 0)
	{
		char address[sizeof(proxy.clientData[0].address)];
		int count = 0;

		GetBaseAddress(Info_ValueForKey(userinfo, "ip"), address, sizeof(address));

		for (int i = 0; i < MAX_CLIENTS; ++i)
		{
			// The address of a slot is empty for a bot, and until the game accepted its client
			if (i != clientNum && !std::strcmp(proxy.clientData[i].address, address) && ++count >= proxy.cvars.proxy_sv_maxClientsPerIP.integer)
			{
				return "Too many connections from your IP address.";
			}
		}
	}

	return nullptr;
}

void Connected(int clientNum, qboolean isBot)
{
	if (isBot)
	{
		return;
	}

	char userinfo[MAX_INFO_STRING];

	proxy.trap->GetUserinfo(clientNum, userinfo, sizeof(userinfo));
	GetBaseAddress(Info_ValueForKey(userinfo, "ip"), proxy.clientData[clientNum].address, sizeof(proxy.clientData[clientNum].address));
}

void Begin(int clientNum)
{
	ClientData* currentClientData = &proxy.clientData[clientNum];

	currentClientData->isConnected = true;

	if (currentClientData->sendWelcomeMessage)
	{
		currentClientData->sendWelcomeMessage = false;

		if (!proxy.cvars.proxy_sv_disableWelcomeMessage.integer)
		{
			proxy.trap->SendServerCommand(clientNum, va("print \"^5%s (^7%s^5) %s^7\n\"", YBEPROXY_NAME, YBEPROXY_VERSION, YBEPROXY_BY_AUTHOR));
		}
	}
}

} // namespace ybeproxy::features::connection
