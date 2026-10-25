#include "JKA_YBEProxy/RuntimePatch/Engine/Proxy_Engine_Wrappers.hpp"
#include "Proxy_common.hpp"

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
// Rewritten whole, Original_Com_WriteConfig_f is never called: on Windows the function starts with a relative call (Cmd_Argc),
// which the trampoline copies without relocating it (HookUtils::GetTramp copies the bytes as they are), so the trampoline must never run
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