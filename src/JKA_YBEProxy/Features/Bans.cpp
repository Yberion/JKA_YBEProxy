#include "JKA_YBEProxy/Proxy.hpp"
#include "JKA_YBEProxy/Features/Bans.hpp"
#include "JKA_YBEProxy/Features/ConsoleCommands.hpp"

/*
IP bans, with the rules of OpenJK's StringToFilter instead of the retail game's

The retail game gives a mask of 255 to an octet only when it is not 0, so a 0 in a ban is a wildcard: "152.0" bans
152.*.*.*, and the ban "152.0.24.34" also bans 152.99.24.34. OpenJK, TaystJK and CleanJK fixed it in the game: every
octet that is written is compared, 0 included, and "*" is the wildcard. "152.0" then bans 152.0.*.* only.
The proxy cannot change the game module, so it keeps the list itself and the game's list stays empty:
- before the game loads, g_banIPs (the game's list, as retail wrote it) is imported and emptied;
- the list is stored in ybeproxy_bans.txt (fs_homepath/<fs_game>/), one ban per line in the new format ("152.0.*.*"),
  read when the game loads and rewritten after every change. The module is reloaded at every map change, so the file
  is read again each time;
- addip, removeip, listip, ban and unban are answered here, and the connection is refused in
  Proxy_SharedAPI_ClientConnect.
Only with g_filterBan 1 (the default). With g_filterBan 0 (only the listed addresses may connect) the game's empty list
would refuse everybody, so the game keeps its own list and its own rules
*/

namespace
{
	// MAX_IPFILTERS of the retail game
	constexpr int MaxIpFilters = 1024;

	constexpr const char* BansFile = "ybeproxy_bans.txt";

	struct IpFilter
	{
		bool			used;
		unsigned char	compare[4];
		unsigned char	mask[4];
	};

	IpFilter	ipFilters[MaxIpFilters];
	int			numIpFilters = 0;
	bool		active = false;

	// legacy: a 0 is a wildcard, as in the retail game. Used for g_banIPs and as a second chance for removeip
	bool StringToFilter(const char* s, IpFilter& f, bool legacy)
	{
		const char* start = s;

		f = IpFilter();

		for (int i = 0; i < 4; ++i)
		{
			if (*s == '*')
			{
				++s;

				if (!*s)
				{
					break;
				}

				if (*s != '.')
				{
					proxy.trap->Print("Bad filter address: %s\n", start);

					return false;
				}

				++s;

				continue;
			}

			if (*s < '0' || *s > '9')
			{
				proxy.trap->Print("Bad filter address: %s\n", start);

				return false;
			}

			int value = 0;

			for (int digits = 0; *s >= '0' && *s <= '9'; ++s)
			{
				value = value * 10 + (*s - '0');

				if (++digits > 3 || value > 255)
				{
					proxy.trap->Print("Bad filter address: %s\n", start);

					return false;
				}
			}

			f.compare[i] = (unsigned char)value;
			f.mask[i] = (legacy && value == 0) ? 0 : 255;

			if (!*s)
			{
				break;
			}

			++s;
		}

		f.used = true;

		return true;
	}

	bool SameFilter(const IpFilter& a, const IpFilter& b)
	{
		return a.used && b.used && !std::memcmp(a.compare, b.compare, 4) && !std::memcmp(a.mask, b.mask, 4);
	}

	// Rewrites the whole file, one ban per line
	void UpdateIPBans(void)
	{
		fileHandle_t f = 0;

		proxy.trap->FS_Open(BansFile, &f, FS_WRITE);

		if (!f)
		{
			proxy.trap->Print("Could not write %s, the change will be lost at the next map change\n", BansFile);

			return;
		}

		for (int i = 0; i < numIpFilters; ++i)
		{
			if (!ipFilters[i].used)
			{
				continue;
			}

			char ip[32] = { 0 };

			for (int j = 0; j < 4; ++j)
			{
				if (ipFilters[i].mask[j] != 255)
				{
					Q_strcat(ip, sizeof(ip), "*");
				}
				else
				{
					Q_strcat(ip, sizeof(ip), va("%i", ipFilters[i].compare[j]));
				}

				Q_strcat(ip, sizeof(ip), (j < 3) ? "." : "\n");
			}

			proxy.trap->FS_Write(ip, (int)std::strlen(ip), f);
		}

		proxy.trap->FS_Close(f);
	}

	// Returns false when the address is not valid or the list is full. Does not save the list
	bool AddIP(const char* str, bool legacy)
	{
		IpFilter filter;

		if (!StringToFilter(str, filter, legacy))
		{
			return false;
		}

		int freeSlot = -1;

		for (int i = 0; i < numIpFilters; ++i)
		{
			if (SameFilter(ipFilters[i], filter))
			{
				return true;
			}

			if (!ipFilters[i].used && freeSlot < 0)
			{
				freeSlot = i;
			}
		}

		if (freeSlot < 0)
		{
			if (numIpFilters == MaxIpFilters)
			{
				proxy.trap->Print("IP filter list is full\n");

				return false;
			}

			freeSlot = numIpFilters++;
		}

		ipFilters[freeSlot] = filter;

		return true;
	}

	// Saves the list and prints the result
	void RemoveIP(const char* str)
	{
		IpFilter filter;

		if (!StringToFilter(str, filter, false))
		{
			return;
		}

		// A ban made before the proxy, "152.0.0.0" for 152.*.*.*, is now "152.*.*.*": the old spelling still finds it
		IpFilter legacyFilter;
		const bool hasLegacy = StringToFilter(str, legacyFilter, true);

		for (int i = 0; i < numIpFilters; ++i)
		{
			if (SameFilter(ipFilters[i], filter) || (hasLegacy && SameFilter(ipFilters[i], legacyFilter)))
			{
				ipFilters[i].used = false;
				proxy.trap->Print("Removed.\n");
				UpdateIPBans();

				return;
			}
		}

		proxy.trap->Print("Didn't find %s.\n", str);
	}

	// The list is words separated by spaces or line breaks
	void AddIPList(const char* list, bool legacy)
	{
		char word[64];
		std::size_t length = 0;

		for (const char* c = list;; ++c)
		{
			if (*c && !std::strchr(" \t\r\n", *c))
			{
				if (length < sizeof(word) - 1)
				{
					word[length++] = *c;
				}

				continue;
			}

			if (length)
			{
				word[length] = '\0';
				AddIP(word, legacy);
				length = 0;
			}

			if (!*c)
			{
				break;
			}
		}
	}
}

void Proxy_Bans_BeforeGameInit(void)
{
	active = false;
	numIpFilters = 0;
	std::memset(ipFilters, 0, sizeof(ipFilters));

	// The game registers g_filterBan with "1". The cvar does not exist yet on a server that never set it
	char filterBan[MAX_CVAR_VALUE_STRING];

	proxy.trap->Cvar_VariableStringBuffer("g_filterBan", filterBan, sizeof(filterBan));

	if (filterBan[0] && !atoi(filterBan))
	{
		proxy.trap->Print("----- Proxy: g_filterBan is 0, the game keeps its own IP ban list\n");

		return;
	}

	// A missing file is an empty list. The buffer holds MaxIpFilters bans as the proxy writes them, with room for "\r\n"
	static char buffer[MaxIpFilters * 20];
	fileHandle_t f = 0;
	const int length = proxy.trap->FS_Open(BansFile, &f, FS_READ);

	if (f)
	{
		if (length > 0)
		{
			// shortcut: a file edited by hand past the buffer is cut at its last full word, read it in chunks if that ever matters
			const int size = (length < (int)sizeof(buffer)) ? length : (int)sizeof(buffer) - 1;

			proxy.trap->FS_Read(buffer, size, f);
			buffer[size] = '\0';

			// The end of the buffer can cut a word, and a cut "152.12" would become the wider ban "152.1": drop that word
			if (size < length)
			{
				int end = size;

				while (end > 0 && !std::strchr(" \t\r\n", buffer[end - 1]))
				{
					--end;
				}

				buffer[end] = '\0';
			}

			AddIPList(buffer, false);
		}

		proxy.trap->FS_Close(f);
	}

	char list[MAX_CVAR_VALUE_STRING];

	// The bans of the game, written by the retail game before the proxy: a 0 was a wildcard
	proxy.trap->Cvar_VariableStringBuffer("g_banIPs", list, sizeof(list));

	if (list[0])
	{
		AddIPList(list, true);
		proxy.trap->Cvar_Set("g_banIPs", "");
		UpdateIPBans();
	}

	active = true;
}

qboolean Proxy_Bans_ConsoleCommand(const char* cmd)
{
	// addip and removeip of the address of a player in the game. Like addip, the player is not dropped: the ban refuses
	// their next connection, at the latest at the next map change
	const bool ban = !Q_stricmp(cmd, "ban");

	if (ban || !Q_stricmp(cmd, "unban"))
	{
		// The game does not know these commands: a dedicated server would print them to every player as a chat line
		if (!active)
		{
			proxy.trap->Print("%s needs g_filterBan 1, use addip and removeip\n", cmd);

			return qtrue;
		}

		const int clientNum = Proxy_SharedAPI_ConsoleClientNum(cmd);

		if (clientNum < 0)
		{
			return qtrue;
		}

		const ClientData_t* clientData = &proxy.clientData[clientNum];

		// Empty for bots
		if (!clientData->address[0])
		{
			proxy.trap->Print("%s ^7has no IP address\n", clientData->cleanName);

			return qtrue;
		}

		if (!ban)
		{
			RemoveIP(clientData->address);
		}
		else if (AddIP(clientData->address, false))
		{
			UpdateIPBans();
			proxy.trap->Print("%s ^7is banned (%s), they will be refused at the next map change or reconnection\n", clientData->cleanName, clientData->address);
		}

		return qtrue;
	}

	if (!active)
	{
		return qfalse;
	}

	char arg[MAX_TOKEN_CHARS];

	if (!Q_stricmp(cmd, "addip"))
	{
		if (proxy.trap->Argc() < 2)
		{
			proxy.trap->Print("Usage: addip <ip-mask>\n");

			return qtrue;
		}

		proxy.trap->Argv(1, arg, sizeof(arg));

		if (AddIP(arg, false))
		{
			UpdateIPBans();
		}

		return qtrue;
	}

	if (!Q_stricmp(cmd, "removeip"))
	{
		if (proxy.trap->Argc() < 2)
		{
			proxy.trap->Print("Usage: removeip <ip-mask>\n");

			return qtrue;
		}

		proxy.trap->Argv(1, arg, sizeof(arg));
		RemoveIP(arg);

		return qtrue;
	}

	if (!Q_stricmp(cmd, "listip"))
	{
		int count = 0;

		for (int i = 0; i < numIpFilters; ++i)
		{
			if (!ipFilters[i].used)
			{
				continue;
			}

			char ip[32] = { 0 };

			for (int j = 0; j < 4; ++j)
			{
				Q_strcat(ip, sizeof(ip), (ipFilters[i].mask[j] != 255) ? "*" : va("%i", ipFilters[i].compare[j]));
				Q_strcat(ip, sizeof(ip), (j < 3) ? "." : "");
			}

			proxy.trap->Print("%s\n", ip);
			++count;
		}

		proxy.trap->Print("%i bans.\n", count);

		return qtrue;
	}

	return qfalse;
}

qboolean Proxy_Bans_IsBanned(const char* ip)
{
	// Not an address, for example "localhost"
	if (!active || *ip < '0' || *ip > '9')
	{
		return qfalse;
	}

	unsigned char in[4] = { 0, 0, 0, 0 };
	int part = 0;

	for (const char* p = ip; *p && part < 4;)
	{
		while (*p >= '0' && *p <= '9')
		{
			in[part] = (unsigned char)(in[part] * 10 + (*p - '0'));
			++p;
		}

		if (!*p || *p == ':')
		{
			break;
		}

		++part;
		++p;
	}

	for (int i = 0; i < numIpFilters; ++i)
	{
		if (!ipFilters[i].used)
		{
			continue;
		}

		bool match = true;

		for (int j = 0; j < 4 && match; ++j)
		{
			match = (in[j] & ipFilters[i].mask[j]) == ipFilters[i].compare[j];
		}

		if (match)
		{
			return qtrue;
		}
	}

	return qfalse;
}
