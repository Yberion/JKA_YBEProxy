#pragma once

// ==================================================
// DetourPatcher by Deathspike, updated by Yberion
// --------------------------------------------------
// Collection of functions written to assist in detouring
// functions and patching bytes. It mainly allocates
// trampolines to execute the original bytes and uses
// jumps to go to the new function. Supports both
// Windows and Linux operating systems.
// ==================================================

namespace HookUtils {
	template <typename FunctionType>
	struct HookEntry {
		const char*		hookName;
		// exemple: Com_Printf address
		unsigned char*	originalFunctionAddr;
		// this variable will store the function ptr that will contain the trampoline address
		// exemple, it will contain the address of "void (*Original_SV_CalcPings)(void);"
		FunctionType**	originalRedirectedFunctionPtr;
		// exemple: Proxy_Com_Printf
		FunctionType*	proxyFunctionPtr;
		size_t			savedOpcodeLength;
		// if we can not allocate, then it is useless to release the memory, in this case, no hook done
		bool			isMemoryAllocated;

		HookEntry(
			const char* hookName,
			unsigned char* originalFunctionAddr,
			FunctionType** originalRedirectedFunctionPtr,
			FunctionType* proxyFunctionPtr,
			size_t savedOpcodeLength
		) :
			hookName(hookName),
			originalFunctionAddr(originalFunctionAddr),
			originalRedirectedFunctionPtr(originalRedirectedFunctionPtr),
			proxyFunctionPtr(proxyFunctionPtr),
			savedOpcodeLength(savedOpcodeLength),
			isMemoryAllocated(false)
		{}
	};

	unsigned char*	 AllocateMemory(const size_t iLen);
	bool			 ReleaseMemory(unsigned char* address, size_t iLen);
	void			 DisAssemble(unsigned char* iptr0, size_t* osizeptr);
	size_t			 GetLen(unsigned char* pAddress);
	unsigned int	 InlineFetch(unsigned char* pAddress);
	unsigned int	 InlinePatch(unsigned char* pAddress, unsigned char* pNewAddress);
	void			 Patch(unsigned char* pAddress, unsigned char bByte);
	void			 Patch_NOP_Bytes(unsigned char* pAddress, size_t iLen);
	void			 ReProtect(void* pAddress, size_t iLen);
	void			 UnProtect(void* pAddress, size_t iLen);

	// ==================================================
	// GetTramp
	// --------------------------------------------------
	// Creates a trampoline, allocates the minimum amount
	// of memory for the length, copies the opcodes to
	// the trampoline and creates a jump to continue
	// execution.
	// ==================================================

	template <typename FunctionType>
	FunctionType* GetTramp(HookEntry<FunctionType>& hookEntry)
	{
		unsigned char* pTramp = AllocateMemory(hookEntry.savedOpcodeLength);

		if (!pTramp)
		{
			return nullptr;
		}

		hookEntry.isMemoryAllocated = true;

		std::memcpy(pTramp, hookEntry.originalFunctionAddr, hookEntry.savedOpcodeLength);
		*(unsigned char*)(pTramp + hookEntry.savedOpcodeLength) = 0xE9;
		*(unsigned int*)(pTramp + hookEntry.savedOpcodeLength + 1) = (unsigned int)((unsigned int)(hookEntry.originalFunctionAddr + hookEntry.savedOpcodeLength) - (unsigned int)(pTramp + hookEntry.savedOpcodeLength + 5));

		return (FunctionType*)pTramp;
	}

	// ==================================================
	// Attach
	// --------------------------------------------------
	// Attaches a detour on the target function and makes 
	// it jump to the new provided address, should be a
	// pointer.
	// ==================================================

	template <typename FunctionType>
	bool Attach(HookEntry<FunctionType>& hookEntry)
	{
		hookEntry.savedOpcodeLength = GetLen(hookEntry.originalFunctionAddr);

		// get the real address of the function ptr where we want to store the trampoline
		*hookEntry.originalRedirectedFunctionPtr = GetTramp(hookEntry);

		if (!*hookEntry.originalRedirectedFunctionPtr)
		{
			return false;
		}

		UnProtect(hookEntry.originalFunctionAddr, hookEntry.savedOpcodeLength);
		std::memset(hookEntry.originalFunctionAddr, 0x90, hookEntry.savedOpcodeLength);

		*((unsigned char*)((unsigned int)hookEntry.originalFunctionAddr)) = 0xE9;
		*((unsigned int*)((unsigned int)hookEntry.originalFunctionAddr + 1)) = (unsigned int)hookEntry.proxyFunctionPtr - (unsigned int)((unsigned int)hookEntry.originalFunctionAddr + 5);
		ReProtect(hookEntry.originalFunctionAddr, hookEntry.savedOpcodeLength);

		return true;
	}

	// ==================================================
	// Detach
	// --------------------------------------------------
	// Detaches a function detour. The original bytes are
	// restored and the tramp is then released.
	// ==================================================

	template <typename FunctionType>
	bool Detach(const HookEntry<FunctionType>& hookEntry)
	{
		if (!hookEntry.isMemoryAllocated)
		{
			return false;
		}

		UnProtect(hookEntry.originalFunctionAddr, hookEntry.savedOpcodeLength);
		std::memcpy((unsigned char*)hookEntry.originalFunctionAddr, (unsigned char*)*hookEntry.originalRedirectedFunctionPtr, hookEntry.savedOpcodeLength);
		ReProtect(hookEntry.originalFunctionAddr, hookEntry.savedOpcodeLength);

		return ReleaseMemory((unsigned char*) *hookEntry.originalRedirectedFunctionPtr, hookEntry.savedOpcodeLength);
	}
}