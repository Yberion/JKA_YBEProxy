# JKA_YBEProxy
Educational project on the creation of a proxy between the server engine and the game module of the game STAR WARS™ Jedi Knight - Jedi Academy™

## Compilation

To compile a 32bits version on a 64bits linux distribution add the following define when generating the CMAKE files:

``cmake .. -DTARGET_ARCH=x86``

Patchnote : [docs/patchnotes.md](docs/patchnotes.md)

Todo : [docs/todo.md](docs/todo.md)

Features : [docs/features.md](docs/features.md)

Trampoline hook : [docs/trampoline-hook.md](docs/trampoline-hook.md)

Audit (fixes, improvements, combat feeling) : [docs/audit/README.md](docs/audit/README.md)

## Notes

If you deal with `static` functions, you better copy/paste them in the Proxy instead of making a trampoline of them because it won't work properly.  
Then after that, each function that call this `static` function should be redefined in a trampoline.  

I'm not sure yet but it seems that sometimes with Discord (and even without) opened it would crash with typing from in-game `rcon map_restart 0` / `rcon map mp/ffa2` (any map) with one of the following hook/trampoline:  

- `SV_PacketEvent`
- `SV_ConnectionlessPacket`
- `SVC_RemoteCommand`
