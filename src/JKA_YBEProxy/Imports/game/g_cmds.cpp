#include "sdk/qcommon/q_shared.hpp"
#include "JKA_YBEProxy/Proxy_Header.hpp"
#include "g_cmds.hpp"

char* ConcatArgs(int start) {
	int		i;
	int		c;
	static char	line[MAX_STRING_CHARS];
	std::size_t	len;
	char	arg[MAX_STRING_CHARS];

	len = 0;
	c = proxy.trap->Argc();

	for (i = start; i < c; i++)
	{
		proxy.trap->Argv(i, arg, sizeof(arg));
		std::size_t tlen = std::strlen(arg);

		if (len + tlen >= MAX_STRING_CHARS - 1)
		{
			break;
		}

		std::memcpy(line + len, arg, tlen);
		len += tlen;

		if (i != c - 1)
		{
			line[len] = ' ';
			len++;
		}
	}

	line[len] = 0;

	return line;
}