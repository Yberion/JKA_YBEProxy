#include "JKA_YBEProxy/Features/Userinfo.hpp"
#include "JKA_YBEProxy/Proxy.hpp"
#include "sdk/server/sv_client.hpp"

namespace ybeproxy::features::userinfo
{

// ==================================================
// Userinfo validation
// --------------------------------------------------
// Port of OpenJK's G_ValidateUserinfo (codemp/game/g_client.c), with its default settings (g_userinfoValidate): size,
// slashes, control characters, and the number of each known key counted without case. Extended ASCII is not checked.
// The info functions of the game read a key without case, but remove only a key with the same case: a userinfo with a
// key twice, or with another case, would let a bad value through the checks below
// The retail client sends every key that is required here, with its default settings
// ==================================================

struct UserinfoField
{
	const char*		name;
	unsigned int	minCount;
	unsigned int	maxCount;
};

static const UserinfoField userinfoFields[] =
{
	{ "cl_guid",		0, 0 }, // not allowed, q3fill protection
	{ "cl_punkbuster",	0, 0 }, // not allowed, q3fill protection
	{ "ip",				0, 1 }, // the engine adds this
	{ "name",			1, 1 },
	{ "rate",			1, 1 },
	{ "snaps",			1, 1 },
	{ "model",			1, 1 },
	{ "forcepowers",	1, 1 },
	{ "color1",			1, 1 },
	{ "color2",			1, 1 },
	{ "handicap",		1, 1 },
	{ "sex",			0, 1 },
	{ "cg_predictItems",1, 1 },
	{ "saber1",			1, 1 },
	{ "saber2",			1, 1 },
	{ "char_color_red",	1, 1 },
	{ "char_color_green",1, 1 },
	{ "char_color_blue",1, 1 },
	{ "teamtask",		0, 1 }, // optional
	{ "password",		0, 1 }, // optional
	{ "teamoverlay",	0, 1 }, // only registered in cgame, not sent when connecting
};

// Returns nullptr, or why the userinfo is not valid. The text of the reason is only valid until the next call
static const char* Validate(const char* userinfo)
{
	static char reason[64];
	const std::size_t length = std::strlen(userinfo);

	if (length < 1)
	{
		return "Userinfo too short";
	}

	if (length >= MAX_INFO_STRING)
	{
		return "Userinfo too long";
	}

	// there must be a leading slash
	if (userinfo[0] != '\\')
	{
		return "Missing leading slash";
	}

	// no trailing slash allowed, the engine adds ip\ip:port in front
	if (userinfo[length - 1] == '\\')
	{
		return "Trailing slash";
	}

	// the format is \key\value, so there must be an even number of slashes
	std::size_t slashes = 0;

	for (std::size_t i = 0; i < length; ++i)
	{
		if (userinfo[i] == '\\')
		{
			++slashes;
		}
	}

	if (slashes & 1)
	{
		return "Bad number of slashes";
	}

	// disallow \n \r ; and \"
	if (Q_strchrs(userinfo, "\n\r;\""))
	{
		return "Invalid characters found";
	}

	unsigned int fieldCount[ARRAY_LEN(userinfoFields)] = {};
	char key[MAX_INFO_KEY];
	char value[MAX_INFO_VALUE];
	const char* s = userinfo;

	while (s)
	{
		Info_NextPair(&s, key, value);

		if (!key[0])
		{
			break;
		}

		for (std::size_t i = 0; i < ARRAY_LEN(userinfoFields); ++i)
		{
			if (!Q_stricmp(key, userinfoFields[i].name))
			{
				++fieldCount[i];
			}
		}
	}

	for (std::size_t i = 0; i < ARRAY_LEN(userinfoFields); ++i)
	{
		if (userinfoFields[i].minCount && !fieldCount[i])
		{
			Com_sprintf(reason, sizeof(reason), "%s field not found", userinfoFields[i].name);

			return reason;
		}

		if (fieldCount[i] > userinfoFields[i].maxCount)
		{
			Com_sprintf(reason, sizeof(reason), "Too many %s fields (%u/%u)", userinfoFields[i].name, fieldCount[i], userinfoFields[i].maxCount);

			return reason;
		}
	}

	return nullptr;
}

// What two names are compared on, as japro and TaystJK (ClientNameEquals): without colors, spaces and case
static void NameKey(const char* name, char* key, std::size_t keySize)
{
	Q_strncpyz(key, name, keySize);
	Q_StripColor(key);

	char* out = key;

	for (const char* in = key; *in; ++in)
	{
		if (*in != ' ')
		{
			*out++ = *in;
		}
	}

	*out = '\0';
}

// A name that another client already uses gets the lowest free number, "Bob" becomes "Bob^7[1]", as japro and TaystJK
// do with g_allowSamePlayerNames 0. The names of the others are those of the game (CS_PLAYERS), bots included
// name: MAX_NETNAME characters
static void UniqueName(int clientNum, char* name)
{
	char others[MAX_CLIENTS][MAX_NETNAME];
	int otherCount = 0;

	for (int i = 0; i < MAX_CLIENTS; ++i)
	{
		char info[MAX_INFO_STRING];

		proxy.trap->GetConfigstring(CS_PLAYERS + i, info, sizeof(info));

		if (i != clientNum && info[0])
		{
			NameKey(Info_ValueForKey(info, "n"), others[otherCount++], sizeof(others[0]));
		}
	}

	const auto inUse = [&](const char* candidate) {
		char key[MAX_NETNAME];

		NameKey(candidate, key, sizeof(key));

		for (int i = 0; i < otherCount; ++i)
		{
			if (!Q_stricmp(key, others[i]))
			{
				return true;
			}
		}

		return false;
	};

	if (!inUse(name))
	{
		return;
	}

	// Room for "^7[32]", and no color escape left alone at the end
	char base[MAX_NETNAME];

	Q_strncpyz(base, name, MAX_NETNAME - 6);

	const std::size_t baseLength = std::strlen(base);

	if (baseLength && base[baseLength - 1] == Q_COLOR_ESCAPE)
	{
		base[baseLength - 1] = '\0';
	}

	for (int number = 1; number <= MAX_CLIENTS; ++number)
	{
		char candidate[MAX_NETNAME];

		Com_sprintf(candidate, sizeof(candidate), "%s" S_COLOR_WHITE "[%i]", base, number);

		if (!inUse(candidate))
		{
			Q_strncpyz(name, candidate, MAX_NETNAME);

			return;
		}
	}
}

const char* Sanitize(int clientNum, bool validate, bool* isAntiCheat)
{
	static char reason[96];
	char userinfo[MAX_INFO_STRING];

	proxy.trap->GetUserinfo(clientNum, userinfo, sizeof(userinfo));

	if (isAntiCheat)
	{
		*isAntiCheat = false;
	}

	if (validate)
	{
		const char* invalid = Validate(userinfo);

		if (invalid)
		{
			Com_sprintf(reason, sizeof(reason), "Failed userinfo validation: %s", invalid);

			return reason;
		}
	}

	if (std::strlen(userinfo) == 0)
	{
		return nullptr;
	}

	std::size_t len = 0;
	const char* val = NULL;

	val = Info_ValueForKey(userinfo, "name");

	ClientData *currentClientData = &proxy.clientData[clientNum];

	// Fix bad names
	Proxy_ClientCleanName(val, currentClientData->cleanName, sizeof(currentClientData->cleanName));

	if (!proxy.cvars.proxy_sv_allowSamePlayerNames.integer)
	{
		UniqueName(clientNum, currentClientData->cleanName);
	}

	// Info_SetValueForKey adds nothing when the userinfo has no room left: the old value would stay, and the game would read it
	if (!Info_SetValueForKey(userinfo, "name", currentClientData->cleanName))
	{
		return "Userinfo string length exceeded.";
	}

	val = Info_ValueForKey(userinfo, "model");

	// Fix bugged models
	// Fix model length crash on some custom clients
	// There's also a check done in SV_UserinfoChanged
	if (val)
	{
		len = std::strlen(val);

		if (!Q_stricmpn(val, "darksidetools", 13))
		{
			if (isAntiCheat)
			{
				*isAntiCheat = true;
			}

			return "(Anti-Cheat system) you got kicked cause of cheating";
		}

		if (!val[0] ||
			!Q_stricmp(val, "jedi_") || !Q_stricmpn(val, "jedi_/", 6) ||
			!Q_stricmpn(val, "rancor", 6) ||
			!Q_stricmpn(val, "wampa", 5) ||
			!Q_IsValidAsciiStr(val) ||
			len >= /*MAX_QPATH*/ (std::size_t)proxy.cvars.proxy_sv_modelPathLength.integer)
		{
			if (!Info_SetValueForKey(userinfo, "model", "kyle"))
			{
				return "Userinfo string length exceeded.";
			}
		}
	}

	//Fix forcepowers crash
	char forcePowers[30];
	
	Q_strncpyz(forcePowers, Info_ValueForKey(userinfo, "forcepowers"), sizeof(forcePowers));

	len = std::strlen(forcePowers);

	qboolean badForce = qfalse;

	if (len >= 22 && len <= 24)
	{
		byte seps = 0;

		for (std::size_t i = 0; i < len; i++)
		{
			if (forcePowers[i] != '-' && (forcePowers[i] < '0' || forcePowers[i] > '9'))
			{
				badForce = qtrue;
				break;
			}

			if ((i < 1 || i > 5) && forcePowers[i] == '-')
			{
				badForce = qtrue;
				break;
			}

			if (i && forcePowers[i - 1] == '-' && forcePowers[i] == '-')
			{
				badForce = qtrue;
				break;
			}

			if (forcePowers[i] == '-')
			{
				seps++;
			}
		}

		if (seps != 2)
		{
			badForce = qtrue;
		}
	}
	else
	{
		badForce = qtrue;
	}

	if (badForce)
	{
		Q_strncpyz(forcePowers, "7-1-030000000000003332", sizeof(forcePowers));
	}

	if (!Info_SetValueForKey(userinfo, "forcepowers", forcePowers))
	{
		return "Userinfo string length exceeded.";
	}

	proxy.trap->SetUserinfo(clientNum, userinfo);

	return nullptr;
}

} // namespace ybeproxy::features::userinfo
