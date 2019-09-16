#include "Proxy_Header.h"

// ==================================================
// server engine
// ==================================================

// playerState_t* SV_GameClientNum(int num)
playerState_t* Proxy_GetPlayerStateByClientNum(int num)
{
	playerState_t* ps;
	
	ps = (playerState_t*)((byte*)proxy.locatedGameData.g_clients + proxy.locatedGameData.g_clientSize * (num));

	return ps;
}

// ==================================================
// q_shared
// ==================================================

/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday
============
*/
#define	MAX_VA_STRING	32000
#define MAX_VA_BUFFERS	4

char *QDECL va(const char* format, ...)
{
	va_list		argptr;
	static char	string[MAX_VA_BUFFERS][MAX_VA_STRING];	// in case va is called by nested functions
	static int	index = 0;
	char* buf;

	va_start(argptr, format);
	buf = (char *)& string[index++ & 3];
	Q_vsnprintf(buf, sizeof(*string), format, argptr);
	va_end(argptr);

	return buf;
}

// ==================================================
// q_string
// ==================================================

#if defined(_MSC_VER)
/*
=============
Q_vsnprintf

Special wrapper function for Microsoft's broken _vsnprintf() function.
MinGW comes with its own snprintf() which is not broken.
=============
*/
int Q_vsnprintf(char* str, size_t size, const char* format, va_list ap)
{
	int retval;

	retval = _vsnprintf(str, size, format, ap);

	if (retval < 0 || retval == size)
	{
		// Microsoft doesn't adhere to the C99 standard of vsnprintf,
		// which states that the return value must be the number of
		// bytes written if the output string had sufficient length.
		//
		// Obviously we cannot determine that value from Microsoft's
		// implementation, so we have no choice but to return size.

		str[size - 1] = '\0';
		return size;
	}

	return retval;
}
#endif

int Q_stricmpn(const char* s1, const char* s2, int n)
{
	int		c1, c2;

	if (s1 == NULL)
	{
		if (s2 == NULL)
			return 0;
		else
			return -1;
	}
	else if (s2 == NULL)
		return 1;

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if (!n--)
		{
			return 0;		// strings are equal until end point
		}

		if (c1 != c2)
		{
			if (c1 >= 'a' && c1 <= 'z')
			{
				c1 -= ('a' - 'A');
			}

			if (c2 >= 'a' && c2 <= 'z')
			{
				c2 -= ('a' - 'A');
			}

			if (c1 != c2) {
				return c1 < c2 ? -1 : 1;
			}
		}
	} while (c1);

	return 0;		// strings are equal
}

// ==================================================
// other
// ==================================================

char* ConcatArgs(int start) {
	int		i, c, tlen;
	static char	line[MAX_STRING_CHARS];
	int		len;
	char	arg[MAX_STRING_CHARS];

	len = 0;
	c = proxy.trap->Argc();

	for (i = start; i < c; i++)
	{
		proxy.trap->Argv(i, arg, sizeof(arg));
		tlen = strlen(arg);

		if (len + tlen >= MAX_STRING_CHARS - 1)
		{
			break;
		}

		memcpy(line + len, arg, tlen);
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