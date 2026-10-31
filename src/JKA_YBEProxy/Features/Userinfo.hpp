#pragma once

namespace ybeproxy::features::userinfo
{

// Checks the userinfo of a client (only if validate), then cleans it in place
// Returns nullptr, or the reason to reject the client. Never drops the client itself
// isAntiCheat, if not null, tells whether the reason is the anti-cheat's
const char* Sanitize(int clientNum, bool validate, bool* isAntiCheat);

} // namespace ybeproxy::features::userinfo
