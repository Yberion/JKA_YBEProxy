#pragma once

extern void (*Original_Cmd_TokenizeString)(const char* text_in);
void Proxy_Cmd_TokenizeString(const char* text_in);

void Proxy_Cmd_TokenizeStringIgnoreQuotes(const char* text_in);
char* Proxy_Cmd_ArgsFrom(int arg);
char* Proxy_Cmd_Cmd(void);
void Cmd_Update_Argv(int argNumber, char* newArg);