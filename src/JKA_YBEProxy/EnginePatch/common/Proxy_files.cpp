#include "JKA_YBEProxy/EnginePatch/Proxy_EnginePatch.hpp"

#include <mutex>

/*
=============
Com_Printf

Both client and server can use this, and it will output
to the apropriate place.

A raw string should NEVER be passed as fmt, because of "%f" type crashers.
=============
*/

void (QDECL *Original_Common_Com_Printf)(const char* fmt, ...);

// Proxy -------------->
std::recursive_mutex printfLock;
// Proxy <--------------
void QDECL Proxy_Common_Com_Printf(const char* fmt, ...)
{
	// Proxy -------------->
	std::lock_guard<std::recursive_mutex> l(printfLock);
	// Proxy <--------------

	static qboolean opening_qconsole = qfalse;
	va_list			argptr;
	char			msg[MAXPRINTMSG];

	va_start(argptr, fmt);
	// Proxy -------------->
	Q_vsnprintf(msg, sizeof(msg), fmt, argptr);
	// Proxy <--------------
	va_end(argptr);

	if (*proxy.server.common.rd_buffer)
	{
		if ((strlen(msg) + strlen(*proxy.server.common.rd_buffer)) > (size_t)(*proxy.server.common.rd_buffersize - 1))
		{
			proxy.server.common.rd_flush(*proxy.server.common.rd_buffer);
			**proxy.server.common.rd_buffer = 0;
		}

		Q_strcat(*proxy.server.common.rd_buffer, *proxy.server.common.rd_buffersize, msg);

		// TTimo nooo .. that would defeat the purpose
		//proxy.server.common.rd_flush(proxy.server.common.rd_buffer);
		//*proxy.server.common.rd_buffer = 0;
		
		return;
	}

	// echo to dedicated console and early console
	proxy.server.common.Sys_Print(msg);

	// logfile
	if (proxy.server.common.com_logfile && proxy.server.common.com_logfile->integer)
	{
		// TTimo: only open the qconsole.log if the filesystem is in an initialized state
		// also, avoid recursing in the qconsole.log opening (i.e. if fs_debug is on)
		
		// Proxy -------------->
		// ( !logfile && FS_Initialized() )
		// Proxy <--------------

		if (!*proxy.server.common.logfile && proxy.server.common.FS_Initialized() && !opening_qconsole)
		{
			struct tm* newtime;
			time_t aclock;

			opening_qconsole = qtrue;

			time(&aclock);
			newtime = localtime(&aclock);

			*proxy.server.common.logfile = proxy.server.common.FS_FOpenFileWrite("qconsole.log");

			// Proxy -------------->
			if (*proxy.server.common.logfile)
			{
				Proxy_Common_Com_Printf("logfile opened on %s\n", asctime(newtime));
				
				if (proxy.server.common.com_logfile->integer > 1)
				{
					// force it to not buffer so we get valid
					// data even if we are crashing
					proxy.server.common.FS_ForceFlush(*proxy.server.common.logfile);
				}
			}
			else
			{
				Proxy_Common_Com_Printf("Opening qconsole.log failed!\n");
				
				Cvar_SetValue("logfile", 0);
			}
		}
		// Proxy <--------------

		opening_qconsole = qfalse;

		if (*proxy.server.common.logfile && proxy.server.common.FS_Initialized())
		{
			proxy.server.common.FS_Write(msg, strlen(msg), *proxy.server.common.logfile);
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