#pragma once

extern void (*Original_SV_Status_f)(void);
void Proxy_SV_Status_f(void);

const char* Proxy_SV_GetStringEdString(const char* refSection, const char* refName);