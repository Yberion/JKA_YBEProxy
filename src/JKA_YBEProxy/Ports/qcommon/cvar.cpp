#include "JKA_YBEProxy/Proxy.hpp"
#include "JKA_YBEProxy/Ports/qcommon/cvar.hpp"

/*
============
Cvar_SetValue
============
*/
cvar_t* Cvar_SetValue(const char* var_name, float value)
{
	char	val[32];

	if (Q_isintegral(value))
		Com_sprintf(val, sizeof(val), "%i", (int)value);
	else
		Com_sprintf(val, sizeof(val), "%f", (double)value);

	proxy.trap->Cvar_Set(var_name, val);

	return nullptr;
}