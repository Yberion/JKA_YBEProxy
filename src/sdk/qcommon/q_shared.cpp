#include "q_shared.hpp"

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
	static std::size_t	index = 0;
	char* buf;

	va_start(argptr, format);
	buf = (char *) &string[index++ & 3];
	Q_vsnprintf(buf, sizeof(*string), format, argptr);
	va_end(argptr);

	return buf;
}

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
FIXME: overflow check?
===============
*/
const char* Info_ValueForKey(const char* s, const char* key) {
	char	pkey[BIG_INFO_KEY];
	static	char value[2][BIG_INFO_VALUE];	// use two buffers so compares
											// work without stomping on each other
	static	std::size_t	valueindex = 0;

	if (!s || !key) {
		return "";
	}

	if (std::strlen(s) >= BIG_INFO_STRING) {
		Com_Error(ERR_DROP, "Info_ValueForKey: oversize infostring");
	}

	valueindex ^= 1;

	if (*s == '\\')
	{
		s++;
	}

	while (1)
	{
		char* o;

		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
			{
				return "";
			}
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value[valueindex];

		while (*s != '\\' && *s)
		{
			*o++ = *s++;
		}
		*o = 0;

		if (!Q_stricmp(key, pkey))
		{
			return value[valueindex];
		}

		if (!*s)
		{
			break;
		}
		s++;
	}

	return "";
}

/*
===================
Info_RemoveKey
===================
*/
void Info_RemoveKey(char* s, const char* key) {
	char* start = NULL;
	char* o = NULL;
	char	pkey[MAX_INFO_KEY] = { 0 };
	char	value[MAX_INFO_VALUE] = { 0 };

	if (std::strlen(s) >= MAX_INFO_STRING) {
		Com_Error(ERR_DROP, "Info_RemoveKey: oversize infostring");
		return;
	}

	if (strchr(key, '\\')) {
		return;
	}

	while (1)
	{
		start = s;
		if (*s == '\\')
		{
			s++;
		}
		o = pkey;
		while (*s != '\\')
		{
			if (!*s)
			{
				return;
			}
			*o++ = *s++;
		}
		*o = 0;
		s++;

		o = value;
		while (*s != '\\' && *s)
		{
			if (!*s)
			{
				return;
			}
			*o++ = *s++;
		}
		*o = 0;

		//OJKNOTE: static analysis pointed out pkey may not be null-terminated
		if (!strcmp(key, pkey))
		{
			memmove(start, s, std::strlen(s) + 1);	// remove this part
			{
				return;
			}
		}

		if (!*s)
		{
			return;
		}
	}
}

/*
==================
Info_SetValueForKey

Changes or adds a key/value pair
==================
*/
void Info_SetValueForKey(char* s, const char* key, const char* value) {
	char	newi[MAX_INFO_STRING];
	const char* blacklist = "\\;\"";

	if (std::strlen(s) >= MAX_INFO_STRING) {
		Com_Error(ERR_DROP, "Info_SetValueForKey: oversize infostring");
	}

	for (; *blacklist; ++blacklist)
	{
		if (strchr(key, *blacklist) || strchr(value, *blacklist))
		{
			Com_Printf(S_COLOR_YELLOW "Can't use keys or values with a '%c': %s = %s\n", *blacklist, key, value);
			return;
		}
	}

	Info_RemoveKey(s, key);
	if (!value || !std::strlen(value))
	{
		return;
	}

	Com_sprintf(newi, sizeof(newi), "\\%s\\%s", key, value);

	if (std::strlen(newi) + std::strlen(s) >= MAX_INFO_STRING)
	{
		Com_Printf("Info string length exceeded: %s\n", s);
		return;
	}

	strcat(newi, s);
	strcpy(s, newi);
}

int QDECL Com_sprintf(char* dest, int size, const char* fmt, ...) {
	int		len;
	va_list		argptr;

	va_start(argptr, fmt);
	len = Q_vsnprintf(dest, size, fmt, argptr);
	va_end(argptr);

	if (len >= size)
	{
		Com_Printf("Com_sprintf: Output length %d too short, require %d bytes.\n", size, len + 1);
	}

	return len;
}