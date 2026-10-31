#pragma once

namespace ybeproxy::engine::hooks
{

extern void (*Original_SV_Status_f)(void);
void Proxy_SV_Status_f(void);

extern void (*Original_SV_DumpUser_f)(void);
void Proxy_SV_DumpUser_f(void);

const char* Proxy_SV_GetStringEdString(const char* refSection, const char* refName);

} // namespace ybeproxy::engine::hooks
