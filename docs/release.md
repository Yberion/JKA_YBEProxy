# Release

How the release archives are built and published, as done for 1.0.0. For the build itself (presets, packages, options), see [Building](../README.md#building) in the README.

## 1. Version

1. Set `YBEPROXY_VERSION` in [src/JKA_YBEProxy/Proxy.hpp](../src/JKA_YBEProxy/Proxy.hpp), for example `"1.0.0"`.
2. In [patchnotes.md](patchnotes.md), rename the `Unreleased` section to the version.
3. Update the example of the `YbeProxy` serverinfo key in [features.md](features.md#proxy_sv_disableserverinfoversion) and in the patch notes, if they show the old version.
4. Commit (`build: version 1.0.0`) and push. The release is built from this commit.

## 2. Windows build

Visual Studio 2026, on Windows:

```
cmake --preset windows-x86
cmake --build --preset windows-x86-release
```

The output is `build/windows-x86/bin/Release/jampgamex86.dll`. The C++ runtime is built into the DLL (`/MT`): it only imports `KERNEL32.dll`, so the server needs no Visual C++ Redistributable.

## 3. Linux build

The `.so` needs a glibc at least as recent as the one of the system it was built on. A build on a recent distribution does not load on older servers: built on Debian 12 (glibc 2.36), the 1.0.0 `.so` would not load on Ubuntu 22.04 or Debian 11 (`version 'GLIBC_2.36' not found`).

The release is built in an Ubuntu 20.04 container (glibc 2.31), with GCC 14 from the [ubuntu-toolchain-r/test](https://launchpad.net/~ubuntu-toolchain-r/+archive/ubuntu/test) PPA. Built that way, the 1.0.0 `.so` needs glibc 2.25 (Debian 10, Ubuntu 18.04, RHEL 8, or later).

On a Linux machine with podman (docker takes the same commands):

```
mkdir ~/ybe-release
podman run --rm -it -v ~/ybe-release:/src docker.io/library/ubuntu:20.04
```

Then in the container:

```
apt update && apt install -y software-properties-common python3-pip git file
add-apt-repository -y ppa:ubuntu-toolchain-r/test && apt update
apt install -y gcc-14 g++-14 g++-14-multilib
pip3 install cmake
git clone --recursive https://github.com/Yberion/JKA_YBEProxy.git /src/JKA_YBEProxy
cd /src/JKA_YBEProxy
git checkout <release commit>
CC=gcc-14 CXX=g++-14 cmake --preset linux-x86-release && cmake --build build/linux-x86-release
```

- `g++-14-multilib` provides the 32-bit libraries for `-m32`. The PPA has GCC 11 to 14 for Ubuntu 20.04, each with its `-multilib` package.
- The CMake of Ubuntu 20.04 is too old (the build needs 3.25 or later): `pip3 install cmake` installs a recent one.
- `~/ybe-release` must be a local folder of the machine. Rootless podman cannot use a VirtualBox shared folder.
- With `--rm`, the container is deleted when you leave it. Only what is in `/src` stays, in `~/ybe-release` on the machine.

The output is `~/ybe-release/JKA_YBEProxy/build/linux-x86-release/bin/jampgamei386.so`. Check it in the container, before you leave it:

```
cd build/linux-x86-release/bin
file jampgamei386.so
objdump -T jampgamei386.so | grep -o 'GLIBC_[0-9.]*' | sort -uV | tail -1
objdump -p jampgamei386.so | grep NEEDED
strings -a jampgamei386.so | grep -Fx 1.0.0
```

- `file`: `ELF 32-bit LSB shared object, Intel 80386`.
- The newest glibc version it needs: `GLIBC_2.25` for 1.0.0. Write it in the release notes.
- `NEEDED`: only libraries of the glibc (`libdl.so.2`, `libm.so.6`, `libpthread.so.0`, `libc.so.6`, `ld-linux.so.2`), no `libstdc++`: the C++ library is built into the `.so`. `libdl.so.2` must be there, it holds `dlopen` before glibc 2.34.
- `strings`: the version of the release, so the build is from the right sources.

## 4. Archives

Two zips, with the layout of the previous releases. The proxy takes the name of the game module, and the original game module is renamed `JKA_YBEProxy` (see [Installation](../README.md#installation)).

`Windows_x86.zip`:

```
Windows_x86/
  base/
    JKA_YBEProxy.dll   the original jampgamex86.dll, renamed
    JKA_YBEProxy.pk3   a zip with the proxy jampgamex86.dll at its root
```

`Linux_x86.zip`:

```
Linux_x86/
  libcxa.so.1          the Intel runtime that the original linuxjampded needs
  base/
    jampgamei386.so    the proxy
    JKA_YBEProxy.so    the original jampgamei386.so, renamed
```

- On Windows, the engine only loads a game DLL from a pk3, so the proxy goes in `JKA_YBEProxy.pk3`.
- The original files come from the 1.01 release of the game and the original Linux server package. They are not in git: keep a copy (the maintainer keeps them in `tmp_src`). Their SHA-1:

  | File | Date | SHA-1 |
  | --- | --- | --- |
  | `jampgamex86.dll` | 2003-11-06 | `af88d9528cb2cd214cfe3520acd5ddd3c54f8a1f` |
  | `jampgamei386.so` | 2003-11-10 | `5adc80bf7309ceded9846d2c6c006451b9326278` |
  | `libcxa.so.1` | 2003-08-07 | `45cb6cff4a53b41f08321e197c5b70481f4c62da` |

- Keep the date of each file in the zip (`cp -p`): the original files show their 2003 date, the proxy the date of its build.

For example, on Linux, with the four files in the current folder (`jampgamex86.dll` and `jampgamei386.so` are the proxy, `retail/` holds the original files):

```
mkdir -p Windows_x86/base Linux_x86/base pk3
cp -p retail/jampgamex86.dll Windows_x86/base/JKA_YBEProxy.dll
cp -p jampgamex86.dll pk3/
(cd pk3 && zip ../Windows_x86/base/JKA_YBEProxy.pk3 jampgamex86.dll)
cp -p jampgamei386.so Linux_x86/base/
cp -p retail/jampgamei386.so Linux_x86/base/JKA_YBEProxy.so
cp -p retail/libcxa.so.1 Linux_x86/
zip -r Windows_x86.zip Windows_x86
zip -r Linux_x86.zip Linux_x86
```

Open each zip afterwards and compare its files with the sources.

## 5. GitHub release

- Tag: the version, without `v` (`1.0.0`), on the release commit of `main`.
- Title: the version.
- Files: `Windows_x86.zip` and `Linux_x86.zip`.
- Notes: links to the patch notes and to the installation, what each zip holds, and the requirements: nothing on Windows, the glibc version found in step 3 on Linux. Links to the documentation can point to the tag (`https://github.com/Yberion/JKA_YBEProxy/blob/1.0.0/...`): they work once the tag exists.
- Not a pre-release.
