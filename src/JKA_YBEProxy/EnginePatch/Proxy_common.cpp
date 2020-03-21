#include "Proxy_EnginePatch.hpp"

/*
=============
Com_Printf

Both client and server can use this, and it will output
to the apropriate place.

A raw string should NEVER be passed as fmt, because of "%f" type crashers.
=============
*/
/*
void QDECL Com_Printf(const char* fmt, ...)
{
	va_list		argptr;
	char		msg[MAXPRINTMSG];
	qboolean	silent;

	va_start(argptr, fmt);
	vsprintf(msg, fmt, argptr);
	va_end(argptr);

	if (rd_buffer)
	{
		if ((strlen(msg) + strlen(rd_buffer)) > (rd_buffersize - 1))
		{
			rd_flush(rd_buffer);
			*rd_buffer = 0;
		}
		Q_strcat(rd_buffer, rd_buffersize, msg);
		rd_flush(rd_buffer);
		*rd_buffer = 0;
		return;
	}

	// * means dont draw this console message on the player screen
	// but put it on the console
	silent = qfalse;
	if (msg[0] == '*')
	{
		strcpy(msg, msg + 1);

		if (msg[1] != '*')
		{
			silent = qtrue;
		}
	}

	// echo to dedicated console and early console
	Sys_Print(msg);

	// logfile
	if (com_logfile && com_logfile->integer)
	{
		if (!logfile && FS_Initialized())
		{
			struct tm* newtime;
			time_t aclock;

			time(&aclock);
			newtime = localtime(&aclock);

			logfile = FS_FOpenFileWrite("qconsole.log");
			Com_Printf("logfile opened on %s\n", asctime(newtime));
			if (com_logfile->integer > 1)
			{
				// force it to not buffer so we get valid
				// data even if we are crashing
				FS_ForceFlush(logfile);
			}
		}
		if (logfile && FS_Initialized())
		{
			FS_Write(msg, strlen(msg), logfile);
		}
	}

#if defined(_WIN32) && defined(_DEBUG)
	if (*msg)
	{
		OutputDebugString(Q_CleanStr(msg));
		OutputDebugString("\n");
	}
#endif
}
*/