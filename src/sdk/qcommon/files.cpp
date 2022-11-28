#include "qcommon.hpp"

/*
================
FS_CheckDirTraversal

Check whether the string contains stuff like "../" to prevent directory traversal bugs
and return qtrue if it does.
================
*/

qboolean FS_CheckDirTraversal(const char* checkdir)
{
	if (strstr(checkdir, "../") || strstr(checkdir, "..\\"))
		return qtrue;

	return qfalse;
}

/*
===========
FS_FilenameCompare

Ignore case and separator char distinctions
===========
*/
qboolean FS_FilenameCompare(const char* s1, const char* s2) {
	int		c1, c2;

	do {
		c1 = *s1++;
		c2 = *s2++;

		if (c1 >= 'a' && c1 <= 'z') {
			c1 -= ('a' - 'A');
		}
		if (c2 >= 'a' && c2 <= 'z') {
			c2 -= ('a' - 'A');
		}

		if (c1 == '\\' || c1 == ':') {
			c1 = '/';
		}
		if (c2 == '\\' || c2 == ':') {
			c2 = '/';
		}

		if (c1 != c2) {
			return qtrue;		// strings not equal
		}
	} while (c1);

	return qfalse;		// strings are equal
}

/*
================
FS_idPak
================
*/
qboolean FS_idPak(char* pak, char* base) {
	int i;

	for (i = 0; i < NUM_ID_PAKS; i++) {
		if (!FS_FilenameCompare(pak, va("%s/assets%d", base, i))) {
			break;
		}
	}
	if (i < NUM_ID_PAKS) {
		return qtrue;
	}
	return qfalse;
}