// ==================================================
// DetourPatcher by Deathspike, updated by Yberion
// --------------------------------------------------
// Collection of functions written to assist in detouring
// functions and patching bytes. It mainly allocates
// trampolines to execute the original bytes and uses
// jumps to go to the new function. Supports both
// Windows and Linux operating systems.
// ==================================================

#ifndef WIN32
	#include <sys/mman.h>
	#include <unistd.h>
#else
	#include <windows.h>
#endif

#include		<cstdlib>
#include		<cstdio>
#include		"HookUtils.hpp"
#include		"LengthDisassembler/hde.hpp"

namespace HookUtils {
	// ==================================================
	// AllocateMemory
	// --------------------------------------------------
	// Use OS specific way of allocating memory when available
	// ==================================================

	unsigned char* AllocateMemory(const std::size_t iLen)
	{
#ifndef WIN32
		//return (unsigned char*)malloc(iLen + 5);
		unsigned char* allocAddr = (unsigned char*)mmap(nullptr, iLen + 5, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

		if (allocAddr == (void*)-1)
		{
			// return nullptr same as VirtualAlloc if failed
			return nullptr;
		}

		return allocAddr;
#else
		return (unsigned char*)VirtualAlloc(0, iLen + 5, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#endif
	}

	// ==================================================
	// ReleaseMemory
	// --------------------------------------------------
	// Use OS specific way of releasing memory when available
	// ==================================================

	bool ReleaseMemory(unsigned char* address, std::size_t iLen)
	{
#ifndef WIN32
		//std::free(address);
		return munmap(address, iLen + 5) == 0;
#else
		// avoid C4100 on MSVC
		iLen = 0;
		return VirtualFree((LPVOID)address, 0, MEM_RELEASE) != 0;
#endif
	}

	// ==================================================
	// GetLen
	// --------------------------------------------------
	// Retrieve the minimum ammount of opcodes to patch
	// at the given instruction. Utilizes the DisAssemble
	// function to count the opcodes.
	// ==================================================

	std::size_t GetLen(unsigned char* pAddress)
	{
		std::size_t iLen = 0;
		std::size_t iSize = 0;
		HDE dummy;

		while (iSize < 5)
		{
			iLen = HDE_DISASM(pAddress, &dummy);
			pAddress += iLen;
			iSize += iLen;
		}

		return iSize;
	}

	// ==================================================
	// InlineFetch
	// --------------------------------------------------
	// Returns the address which is stored in anything
	// similar to a jump or call (4 bytes address after
	// a single opcode).
	// ==================================================

	uintptr_t InlineFetch(unsigned char* pAddress)
	{
		uintptr_t* pAddressBase = (uintptr_t*)(pAddress + 5);
		uintptr_t pAddressOffset = *(uintptr_t*)(pAddress + 1);
		return (uintptr_t)pAddressBase + (uintptr_t)pAddressOffset;
	}

	// ==================================================
	// InlinePatch
	// --------------------------------------------------
	// Patches the address which is stored in anything
	// similar to a jump or call (4 bytes address after
	// a single opcode).
	// ==================================================

	uintptr_t InlinePatch(unsigned char* pAddress, unsigned char* pNewAddress)
	{
		uintptr_t pReturnAddress = InlineFetch(pAddress);

		UnProtect(pAddress, 5);
		*(uintptr_t*)(pAddress + 1) = (std::ptrdiff_t)((uintptr_t)pNewAddress - (uintptr_t)(pAddress + 5));
		ReProtect(pAddress, 5);

		return pReturnAddress;
	}

	// ==================================================
	// Patch
	// --------------------------------------------------
	// Patches a single byte at the target address, 
	// usually performed to skip instructions from that 
	// given address.
	// ==================================================

	void Patch(unsigned char* pAddress, unsigned char bByte)
	{
		UnProtect(pAddress, 1);
		*pAddress = bByte;
		ReProtect(pAddress, 1);
	}

	// ==================================================
	// Patch_NOP_Bytes
	// --------------------------------------------------
	// Patches X bytes to NOP at the target address, 
	// usually performed to skip instructions from that 
	// given address.
	// ==================================================

	void Patch_NOP_Bytes(unsigned char* pAddress, std::size_t iLen)
	{
		UnProtect(pAddress, iLen);
		std::memset(pAddress, 0x90, iLen);
		ReProtect(pAddress, iLen);
	}

	// ==================================================
	// ReProtect
	// --------------------------------------------------
	// Reapplies protection to the target address, must
	// be called after any UnProtect call!
	// ==================================================

	void ReProtect(void* pAddress, std::size_t iLen)
	{
#ifndef WIN32
		uintptr_t iPage1 = (uintptr_t)pAddress & ~(getpagesize() - 1);
		uintptr_t iPage2 = ((uintptr_t)pAddress + iLen) & ~(getpagesize() - 1);

		if (iPage1 == iPage2)
		{
			mprotect((unsigned char*)iPage1, getpagesize(), PROT_READ | PROT_EXEC);
		}
		else
		{
			mprotect((unsigned char*)iPage1, getpagesize(), PROT_READ | PROT_EXEC);
			mprotect((unsigned char*)iPage2, getpagesize(), PROT_READ | PROT_EXEC);
		}
#else
		DWORD dummy;
		VirtualProtect(pAddress, iLen, PAGE_EXECUTE_READ, &dummy);
#endif
	}

	// ==================================================
	// UnProtect
	// --------------------------------------------------
	// Removes protection to the target address, after
	// which it can be written upon. Do not open an entire
	// page. On linux we have to determine whether or not
	// the entire length is on the same page.
	// ==================================================

	void UnProtect(void* pAddress, std::size_t iLen)
	{
#ifndef WIN32

		uintptr_t iPage1 = (uintptr_t)pAddress & ~(getpagesize() - 1);
		uintptr_t iPage2 = ((uintptr_t)pAddress + iLen) & ~(getpagesize() - 1);

		if (iPage1 == iPage2)
		{
			mprotect((unsigned char*)iPage1, getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC);
		}
		else
		{
			mprotect((unsigned char*)iPage1, getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC);
			mprotect((unsigned char*)iPage2, getpagesize(), PROT_READ | PROT_WRITE | PROT_EXEC);
		}

#else
		DWORD dummy;
		VirtualProtect(pAddress, iLen, PAGE_EXECUTE_READWRITE, &dummy);
#endif
	}
}