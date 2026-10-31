#pragma once

namespace ybeproxy::engine::hooks
{

void Proxy_Windows_Com_EndRedirect(void);

extern void (*Original_Com_WriteConfig_f)(void);
void Proxy_Com_WriteConfig_f(void);

extern void (*Original_Info_Print)(const char*);
void Proxy_Info_Print(const char* s);

} // namespace ybeproxy::engine::hooks
