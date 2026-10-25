#pragma once

void Proxy_Windows_Com_EndRedirect(void);

extern void (*Original_Com_WriteConfig_f)(void);
void Proxy_Com_WriteConfig_f(void);