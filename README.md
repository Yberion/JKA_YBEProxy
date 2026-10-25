# JKA_YBEProxy

A proxy game module for the STAR WARS™ Jedi Knight: Jedi Academy™ multiplayer server.

The proxy sits between the server engine (`jampDed.exe` / `linuxjampded`) and the original game module (`jampgame`). It intercepts the calls in both directions and patches the engine at runtime. This fixes crashes and exploits and adds features **without recompiling the game module**, so gameplay stays identical to the retail game.

This is an educational project.

## Documentation

- [Features](docs/features.md): cvars, commands and every automatic fix.
- [Patch notes](docs/patchnotes.md)
- [Todo](docs/todo.md)
- [Trampoline hooks](docs/trampoline-hook.md): how the engine hooks work.
- [Audit](docs/audit-2026-10-25/README.md): known bugs, improvements, and the combat "feeling" investigation.

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
| OpenJK | Not checked | Game module layer only, through `dllEntry` and `vmMain` (OpenJK's legacy game API) |

> [!WARNING]
> OpenJK support is not tested yet, neither with the retail game module nor with OpenJK's.

The original engine and the retail game module are 32-bit, so use the 32-bit (x86) build of the proxy with them.

## Installation

The proxy loads the original game module from `<fs_game>/JKA_YBEProxy.<ext>`, or `base/JKA_YBEProxy.<ext>` when `fs_game` is empty.

- This path is relative to `fs_basepath`, not to `fs_homepath`. By default `fs_basepath` is the server's current working directory, so start the server from the game's root folder, or set `fs_basepath`.
- If the file cannot be opened, the server exits and prints the full path it tried.
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

Requirements: CMake 3.12 or later and a C++20 compiler (Visual Studio 2026, GCC or Clang). Without an explicit build type, CMake builds in Release.

### Windows

Requirements: Visual Studio 2026 and CMake 4.2 or later, the first version with the Visual Studio 2026 generator.

Run `CreateVisualStudio2026Projects_x86.bat`. It generates a Visual Studio 2026 solution in `build/` (Win32, static runtime):

```
cmake -G "Visual Studio 18 2026" -A Win32 -T host=x86 -D MSVC_RUNTIME=static ..
```

If `build/` was generated for another Visual Studio version, delete it first: CMake does not change the generator of an existing build folder.

Open the `JKA_YBEProxy` solution in `build/` and build the Release configuration. The output is `build/Release/jampgamex86.dll`.

`CreateVisualStudio2026Projects_x64.bat` produces a 64-bit build. It is only useful with a 64-bit OpenJK server, not with the original engine.

### Linux

The original engine is 32-bit, so on a 64-bit distribution the proxy is built in 32-bit (`-DTARGET_ARCH=x86`). This needs the 32-bit C library and the 32-bit C++ library for GCC.

Debian, Ubuntu (`apt`):

```
sudo apt install build-essential cmake git gcc-multilib g++-multilib
```

- `build-essential`: `gcc`, `g++`, `make` and the C library headers.
- `gcc-multilib`, `g++-multilib`: the 32-bit C and C++ libraries for `-m32`, static C++ library included.

Fedora, RHEL, Rocky Linux, AlmaLinux (`dnf`):

```
sudo dnf install gcc gcc-c++ make cmake git glibc-devel.i686 libstdc++-devel.x86_64 libstdc++-devel.i686 libstdc++-static.i686 libatomic.i686
```

- `glibc-devel.i686`: the 32-bit C library.
- `libstdc++-devel.x86_64`: the C++ headers, including the 32-bit `bits/c++config.h` that `g++ -m32` uses (`x86_64-redhat-linux/32`). Name it explicitly: `libstdc++-devel.i686` alone satisfies the dependency of `gcc-c++`, and then `-m32` fails with `bits/c++config.h: No such file or directory`.
- `libstdc++-devel.i686`: the 32-bit C++ library.
- `libstdc++-static.i686`: the 32-bit static C++ library, linked into the `.so`.
- `libatomic.i686`: since GCC 16, `g++` always passes `-latomic` to the linker (`--as-needed`), so the 32-bit one must exist. Without it: `cannot find /usr/lib/libatomic.so.1.2.0`.

Build:

```
mkdir build && cd build
cmake .. -DTARGET_ARCH=x86
make
```

The output is `build/src/jampgamei386.so`. `file src/jampgamei386.so` must show `ELF 32-bit LSB shared object, Intel 80386`.

`libstdc++` and `libgcc` are linked into the `.so`, so the server needs no C++ library. It still needs a glibc at least as recent as the one the `.so` was built with, otherwise the server cannot load it (`version 'GLIBC_2.xx' not found`). Build on the server itself, or on a distribution that is not newer than the server's.

## Developer notes

- Check the disassembly before hooking a `static` function of the original binaries. The compiler may have inlined it into its callers, or given it a non-standard calling convention, and in both cases a plain trampoline hook does not work. See the pitfalls in [docs/trampoline-hook.md](docs/trampoline-hook.md#5-pitfalls).
- Crash on `rcon map <map>` and `rcon map_restart 0`, observed more often with Discord running on the server machine. The likely cause: hook functions were still on the call stack when the engine unloaded the proxy. The out-of-band packet checks now run in a filter at the entry of `SV_PacketEvent`, which returns before the original code runs (F1 in [docs/audit-2026-10-25/fixes.md](docs/audit-2026-10-25/fixes.md)). The crash was never reproduced, so the fix is not confirmed yet.

## Credits

- [JMPProxy](https://github.com/Deathspike/JMPProxy) by Deathspike: original idea and structure, and the DetourPatcher the hook utilities are based on.
- Assembly shell system by Deathspike and BobaFett, with Sil's fix.
- [OpenJK](https://github.com/JACoders/OpenJK): several fixes and parts of the code.
- [jk2mv](https://github.com/mvdevs/jk2mv) and fau: packet and FPS statistics, ping fix.
- Hacker Disassembler Engine (HDE32/HDE64) by Vyacheslav Patkov: instruction length disassembler.
- Original game source code: [jediacademy](https://github.com/jedis/jediacademy) by Raven Software and Activision.

## License

GNU GPL v2, see [LICENSE](LICENSE). See [rv-readme.txt](rv-readme.txt) for the notice of the original source release. Third-party files keep the license stated in their headers.
