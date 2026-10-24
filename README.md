# JKA_YBEProxy

A proxy game module for the STAR WARS™ Jedi Knight: Jedi Academy™ multiplayer server.

The proxy sits between the server engine (`jampDed.exe` / `linuxjampded`) and the original game module (`jampgame`). It intercepts the calls in both directions and patches the engine at runtime. This fixes crashes and exploits and adds features **without recompiling the game module**, so gameplay stays identical to the retail game.

This is an educational project.

## How it works

- The engine loads the proxy as if it were the game module (`jampgamex86.dll` / `jampgamei386.so`).
- On `GAME_INIT`, the proxy loads the original game module, renamed to `JKA_YBEProxy.dll` / `JKA_YBEProxy.so`.
- The proxy forwards every call in both directions:
  - engine to game: engine, then proxy `vmMain`, then original `vmMain`;
  - game to engine: original game, then proxy syscall handler, then engine.

  On the way, it can inspect, modify or block any call.
- If the engine is the original 1.01 binary, the proxy also hooks and patches engine functions in memory. See [docs/trampoline-hook.md](docs/trampoline-hook.md).
- On `GAME_SHUTDOWN`, the proxy removes its engine patches and unloads the original game module.

## Supported servers

| Engine | `version` string used for detection | What the proxy does |
| --- | --- | --- |
| Original Windows `jampDed.exe` 1.01 | `(internal)JAmp: v1.0.1.0 win-x86 Oct 30 2003` | Game module layer and engine patches |
| Original Linux `linuxjampded` 1.01 | `JAmp: v1.0.1.1 linux-i386 Nov 10 2003` | Game module layer and engine patches |
| OpenJK | Not checked | Game module layer only, through the OpenJK API (`GetModuleAPI`) |

> [!WARNING]
> OpenJK support has known issues: proxy cvars are never registered, and OpenJK with the retail game module fails to start. See F4 in [docs/audit/fixes.md](docs/audit/fixes.md) and A2 in [docs/audit/improvements.md](docs/audit/improvements.md).

The original engine and the retail game module are 32-bit, so use the 32-bit (x86) build of the proxy with them.

## Installation

The proxy loads the original game module from `<fs_game>/JKA_YBEProxy.<ext>`, or `base/JKA_YBEProxy.<ext>` when `fs_game` is empty.

- This path is relative to the server's current working directory, not to `fs_basepath` / `fs_homepath`. Start the server from the game's root folder.
- The original game module must be a plain file at that path, not inside a pk3.

### Windows

1. Rename the original `jampgamex86.dll` in `base` to `JKA_YBEProxy.dll`. When `fs_game` is set, put it in the `fs_game` folder instead.
2. Put the proxy `jampgamex86.dll` **inside a `.pk3`** in the same folder.
   - Why: the engine reads the game DLL through its virtual filesystem and extracts it to disk before loading it. With `sv_pure 1`, only files inside pk3 archives are visible, so a loose DLL is not found.
   - In `base`, give the pk3 a name that sorts after the retail pk3 files, for example `zz_ybeproxy.pk3`. When several pk3 files contain the same file, the last one in alphabetical order wins.

### Linux

1. In your `fs_game` folder, or in `base`, rename the original `jampgamei386.so` to `JKA_YBEProxy.so`.
2. Copy the proxy `jampgamei386.so` into the same folder.

### Check

Check the server console. You should see lines like:

   ```
   ----- Proxy: YbeProxy 0.19.0 Beta by Yberion
   ----- Proxy: JKA_YBEProxy.dll properly loaded
   ----- Proxy: Original engine detected
   ----- Proxy: Engine properly patched
   ```

### Updating without restarting the server

The engine reloads the game module on every map change.

- Linux: replace the `.so` with `mv`. The new build is used from the next map change. **Never `cp` over a loaded `.so`: the running server crashes with SIGBUS.**
- Windows: the proxy comes from a pk3 that the server keeps open, and the engine extracts the DLL from it at every load. Replacing that pk3 while the server runs has not been tested. Restart the server to be safe.

## Building

Requirements: CMake 3.12 or later and a C++20 compiler (Visual Studio 2022, GCC or Clang). Without an explicit build type, CMake builds in Release.

### Windows

Run `CreateVisualStudio2022Projects_x86.bat`. It generates a Visual Studio 2022 solution in `build/` (Win32, static runtime):

```
cmake -G "Visual Studio 17" -A Win32 -T host=x86 -D MSVC_RUNTIME=static ..
```

Open `build/JKA_YBEProxy.sln` and build the Release configuration. The output is `build/Release/jampgamex86.dll`.

`CreateVisualStudio2022Projects_x64.bat` produces a 64-bit build. It is only useful with a 64-bit OpenJK server, not with the original engine.

### Linux

To build the 32-bit version on a 64-bit distribution, pass `-DTARGET_ARCH=x86` (this needs 32-bit compiler support, for example `gcc-multilib` and `g++-multilib`):

```
mkdir build && cd build
cmake .. -DTARGET_ARCH=x86
make
```

The output is `jampgamei386.so`.

## Documentation

- [Features](docs/features.md): cvars, commands and every automatic fix.
- [Patch notes](docs/patchnotes.md)
- [Todo](docs/todo.md)
- [Trampoline hooks](docs/trampoline-hook.md): how the engine hooks work.
- [Audit](docs/audit/README.md): known bugs, improvements, and the combat "feeling" investigation.

## Developer notes

- Check the disassembly before hooking a `static` function of the original binaries. The compiler may have inlined it into its callers, or given it a non-standard calling convention, and in both cases a plain trampoline hook does not work. See the pitfalls in [docs/trampoline-hook.md](docs/trampoline-hook.md#5-pitfalls).
- Known crash: `rcon map <map>` and `rcon map_restart 0` sometimes crash the server, which was observed more often with Discord running on the server machine. The cause is identified: hook functions are still on the call stack when the engine unloads the proxy. The fix is described in F1 of [docs/audit/fixes.md](docs/audit/fixes.md).

## Credits

- [JMPProxy](https://github.com/Deathspike/JMPProxy) by Deathspike: original idea and structure, and the DetourPatcher the hook utilities are based on.
- Assembly shell system by Deathspike and BobaFett, with Sil's fix.
- [OpenJK](https://github.com/JACoders/OpenJK): several fixes and parts of the code.
- [jk2mv](https://github.com/mvdevs/jk2mv) and fau: packet and FPS statistics, ping fix.
- Hacker Disassembler Engine (HDE32/HDE64) by Vyacheslav Patkov: instruction length disassembler.
- Original game source code: [jediacademy](https://github.com/jedis/jediacademy) by Raven Software and Activision.

## License

GNU GPL v2, see [LICENSE](LICENSE). See [rv-readme.txt](rv-readme.txt) for the notice of the original source release. Third-party files keep the license stated in their headers.
