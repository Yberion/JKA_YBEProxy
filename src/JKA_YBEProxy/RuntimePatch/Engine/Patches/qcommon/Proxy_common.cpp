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