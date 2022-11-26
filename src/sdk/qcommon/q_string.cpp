#include "q_shared.hpp"
#include "q_string.hpp"

#if defined(_MSC_VER)
/*
=============
Q_vsnprintf

Special wrapper function for Microsoft's broken _vsnprintf() function.
MinGW comes with its own snprintf() which is not broken.
=============
*/
int Q_vsnprintf(char* str, std::size_t size, const char* format, va_list args)
{
	int retval;

	retval = _vsnprintf(str, size, format, args);

	if (retval < 0 || retval == size)
	{
		// Microsoft doesn't adhere to the C99 standard of vsnprintf,
		// which states that the return value must be the number of
		// bytes written if the output string had sufficient length.
		//
		// Obviously we cannot determine that value from Microsoft's
		// implementation, so we have no choice but to return size.

		str[size - 1] = '\0';
		return (int)size;
	}

	return retval;
}
#endif

int Q_stricmpn(const char* s1, const char* s2, std::size_t n)
{
	int		c1;

	if (s1 == NULL)
	{
		if (s2 == NULL)
		{
			return 0;
		}

		return -1;
	}
	else if (s2 == NULL)
	{
		return 1;
	}

	do
	{
		c1 = *s1++;
		int c2 = *s2++;

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

int Q_stricmp(const char* s1, const char* s2) {
	return (s1 && s2) ? Q_stricmpn(s1, s2, 99999) : -1;
}

int Q_strncmp(const char* s1, const char* s2, int n) {
	int		c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if (!n--) {
			return 0;		// strings are equal until end point
		}

		if (c1 != c2) {
			return c1 < c2 ? -1 : 1;
		}
	} while (c1);

	return 0;		// strings are equal
}

/*
Q_strchrs

Description:	Find any characters in a string. Think of it as a shorthand strchr loop.
Mutates:		--
Return:			first instance of any characters found
otherwise NULL
*/

const char* Q_strchrs(const char* string, const char* search)
{
	const char* p = string;
	const char *s;

	while (*p != '\0')
	{
		for (s = search; *s; s++)
		{
			if (*p == *s)
			{
				return p;
			}
		}

		p++;
	}

	return NULL;
}

/*
=============
Q_strncpyz

Safe strncpy that ensures a trailing zero
=============
*/
void Q_strncpyz(char* dest, const char* src, std::size_t destsize) {
	assert(src);
	assert(dest);
	assert(destsize);

	strncpy(dest, src, destsize - 1);
	dest[destsize - 1] = 0;
}

// never goes past bounds or leaves without a terminating 0
void Q_strcat(char* dest, std::size_t size, const char* src)
{
	std::size_t		l1;

	l1 = std::strlen(dest);
	if (l1 >= size)
	{
		//Com_Error( ERR_FATAL, "Q_strcat: already overflowed" );
		return;
	}
	if (std::strlen(src) + 1 > (std::size_t)(size - l1))
	{	//do the error here instead of in Q_strncpyz to get a meaningful msg
		//Com_Error(ERR_FATAL,"Q_strcat: cannot append \"%s\" to \"%s\"", src, dest);
		return;
	}
	Q_strncpyz(dest + l1, src, size - l1);
}

char* Q_CleanStr(char* string)
{
	char* d;
	char* s;
	int		c;

	s = string;
	d = string;
	while ((c = *s) != 0)
	{
		if (Q_IsColorString(s))
		{
			s++;
		}
		else if (c >= 0x20 && c <= 0x7E)
		{
			*d++ = c;
		}
		s++;
	}
	*d = '\0';

	return string;
}

char* Q_CleanAsciiStr(char* string) {
	char* d;
	char* s;
	int	c;

	s = string;
	d = string;
	while ((c = *s) != 0) {
		if (c >= 0x20 && c <= 0x7E) {
			*d++ = c;
		}
		s++;
	}
	*d = '\0';

	return string;
}

qboolean Q_IsValidAsciiStr(const char* string) {
	const char* s;
	int	c;

	s = string;

	while ((c = *s) != 0) {
		if (c < 0x20 || c > 0x7E) {
			return qfalse;
		}
		s++;
	}

	return qtrue;
}

qboolean Q_isintegral(float f)
{
	return (qboolean)((int)f == f);
}

/*
==================
Q_StripColor

Strips coloured strings in-place using multiple passes: "fgs^^56fds" -> "fgs^6fds" -> "fgsfds"

This function modifies INPUT (is mutable)

(Also strips ^8 and ^9)
==================
*/
void Q_StripColor(char* text)
{
	qboolean doPass = qtrue;
	char* read;
	char* write;

	while (doPass)
	{
		doPass = qfalse;
		read = write = text;
		while (*read)
		{
			if (Q_IsColorStringExt(read))
			{
				doPass = qtrue;
				read += 2;
			}
			else
			{
				// Avoid writing the same data over itself
				if (write != read)
				{
					*write = *read;
				}
				write++;
				read++;
			}
		}
		if (write < read)
		{
			// Add trailing NUL byte if string has shortened
			*write = '\0';
		}
	}
}
