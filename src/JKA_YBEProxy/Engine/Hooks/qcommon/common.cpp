#include "JKA_YBEProxy/Engine/EngineState.hpp"
#include "JKA_YBEProxy/Engine/Hooks/qcommon/common.hpp"

namespace ybeproxy::engine::hooks
{

void Proxy_Windows_Com_EndRedirect(void)
{

	if (*server.common.vars.rd_flush) {
		(*server.common.vars.rd_flush)(*server.common.vars.rd_buffer);
	}

	*server.common.vars.rd_buffer = NULL;
	*server.common.vars.rd_buffersize = 0;
	*server.common.vars.rd_flush = NULL;
}

/*
===============
Com_WriteConfig_f

Write the config file to a specific name
===============
*/
// Rewritten whole, Original_Com_WriteConfig_f is never called: nothing from the original function is needed
// (on Windows it starts with a relative call to Cmd_Argc: HookUtils::Attach relocates it in the trampoline)
void (*Original_Com_WriteConfig_f)(void);
void Proxy_Com_WriteConfig_f(void)
{
	char filename[MAX_QPATH];

	if (*server.common.vars.cmd_argc != 2)
	{
		server.common.functions.Com_Printf("Usage: writeconfig <filename>\n");
		return;
	}

	Q_strncpyz(filename, server.common.functions.Cmd_Argv(1), sizeof(filename));
	COM_DefaultExtension(filename, sizeof(filename), ".cfg");

	// Proxy -------------->
	// From OpenJK: .cfg only, and not the default configs. The check is in the command itself, so vstr and exec can't bypass it
	if (!COM_CompareExtension(filename, ".cfg"))
	{
		server.common.functions.Com_Printf("Com_WriteConfig_f: Only the \".cfg\" extension is supported by this command!\n");
		return;
	}

	// FS_FOpenFileWrite checks nothing: without this, "../" writes outside fs_homepath/fs_game
	if (FS_CheckDirTraversal(filename))
	{
		server.common.functions.Com_Printf("Com_WriteConfig_f: The filename \"%s\" must not contain \"..\"!\n", filename);
		return;
	}

	if (!FS_FilenameCompare(filename, "mpdefault.cfg") || !FS_FilenameCompare(filename, "default.cfg"))
	{
		server.common.functions.Com_Printf(S_COLOR_YELLOW "Com_WriteConfig_f: The filename \"%s\" is reserved! Please choose another name.\n", filename);
		return;
	}
	// Proxy <--------------

	server.common.functions.Com_Printf("Writing %s.\n", filename);
	server.common.functions.Com_WriteConfigToFile(filename);
}

/*
===============
Info_Print

Used by the serverinfo, systeminfo and dumpuser commands
===============
*/
// Rewritten whole, Original_Info_Print is never called
void (*Original_Info_Print)(const char*);
void Proxy_Info_Print(const char* s)
{
	// Proxy -------------->
	// The original buffers are 512 chars and the copies below are not bounded: a key or value of 512 chars or more
	// overflows them. dumpuser prints the userinfo, which comes from the client
	// Sizes from ioq3 ("Fix systeminfo command"), as in OpenJK. The strings printed here are at most MAX_INFO_STRING
	//char	key[512];
	//char	value[512];
	char	key[BIG_INFO_KEY];
	char	value[BIG_INFO_VALUE];
	// Proxy <--------------
	char	*o;
	int		l;

	if (*s == '\\')
		s++;
	while (*s)
	{
		o = key;
		while (*s && *s != '\\')
			*o++ = *s++;

		l = o - key;
		if (l < 20)
		{
			std::memset(o, ' ', 20 - l);
			key[20] = 0;
		}
		else
			*o = 0;
		server.common.functions.Com_Printf("%s", key);

		if (!*s)
		{
			server.common.functions.Com_Printf("MISSING VALUE\n");
			return;
		}

		o = value;
		s++;
		while (*s && *s != '\\')
			*o++ = *s++;
		*o = 0;

		if (*s)
			s++;
		server.common.functions.Com_Printf("%s\n", value);
	}
}

} // namespace ybeproxy::engine::hooks
