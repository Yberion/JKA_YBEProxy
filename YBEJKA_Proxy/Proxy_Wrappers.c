#include "Proxy_Header.h"

int QDECL Proxy_systemCall(int command, int* arg1, int* arg2, int* arg3, int* arg4, int* arg5, int* arg6, int* arg7, int* arg8, int* arg9, int* arg10)
{
	switch (command)
	{
		default:
			break;
	}

	return proxy.originalSystemCall(command, arg1, arg2, arg3, arg4, arg5, arg6, arg7, arg8, arg9, arg10);
}