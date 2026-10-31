#pragma once

// Checks the userinfo of a client (only if validate), then cleans it in place
// Returns nullptr, or the reason to reject the client. Never drops the client itself
// isAntiCheat, if not null, tells whether the reason is the anti-cheat's
const char* Proxy_SharedAPI_SanitizeUserinfo(int clientNum, bool validate, bool* isAntiCheat);
