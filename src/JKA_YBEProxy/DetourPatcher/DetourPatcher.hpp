#pragma once

// ==================================================
// DetourPatcher by Deathspike, modified by Yberion
// --------------------------------------------------
// Collection of functions written to assist in detouring
// functions and patching bytes. It mainly allocates
// trampolines to execute the original bytes and uses
// jumps to go to the new function. Supports both
// Windows and Linux operating systems.
// ==================================================

unsigned char*	 AllocateMemory(size_t iLen);
void			 ReleaseMemory(unsigned char* address, size_t iLen);
void			 DisAssemble( unsigned char *iptr0, size_t *osizeptr );
size_t			 GetLen( unsigned char *pAddress );
unsigned int	 InlineFetch( unsigned char *pAddress );
unsigned int	 InlinePatch( unsigned char *pAddress, unsigned char *pNewAddress );
void			 Patch( unsigned char *pAddress, unsigned char bByte );
void			 Patch_NOP_Bytes(unsigned char* pAddress, size_t iLen);
void			 ReProtect( void *pAddress, size_t iLen );
void			 UnProtect( void *pAddress, size_t iLen );

namespace HookUtils {
	template <typename FunctionType>
	struct HookEntry {
		// exemple: Com_Printf address
		unsigned char*	originalFunctionAddr;
		// this variable will store the function ptr that will contain the trampoline address
		// exemple, it will contain the address of "void (*Original_SV_CalcPings)(void);"
		FunctionType**	originalRedirectedFunctionPtr;
		// exemple: Proxy_Com_Printf
		FunctionType*	proxyFunctionPtr;
		size_t			savedOpcodeLength;

		HookEntry(
			unsigned char* originalFunctionAddr,
			FunctionType** originalRedirectedFunctionPtr,
			FunctionType* proxyFunctionPtr,
			size_t savedOpcodeLength
		) :
			originalFunctionAddr(originalFunctionAddr),
			originalRedirectedFunctionPtr(originalRedirectedFunctionPtr),
			proxyFunctionPtr(proxyFunctionPtr),
			savedOpcodeLength(savedOpcodeLength)
		{}
	};

	// ==================================================
	// GetTramp
	// --------------------------------------------------
	// Creates a trampoline, allocates the minimum amount
	// of memory for the length, copies the opcodes to
	// the trampoline and creates a jump to continue
	// execution.
	// ==================================================

	template <typename FunctionType>
	FunctionType* GetTramp(unsigned char* pAddress, size_t iLen)
	{
		unsigned char* pTramp = AllocateMemory(iLen);

		std::memcpy(pTramp, pAddress, iLen);
		*(unsigned char*)(pTramp + iLen) = 0xE9;
		*(unsigned int*)(pTramp + iLen + 1) = (unsigned int)((unsigned int)(pAddress + iLen) - (unsigned int)(pTramp + iLen + 5));

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
	void Attach(HookEntry<FunctionType>& hookEntry)
	{
		hookEntry.savedOpcodeLength = GetLen(hookEntry.originalFunctionAddr);

		// get the real address of the function ptr where we want to store the trampoline
		*hookEntry.originalRedirectedFunctionPtr = GetTramp<FunctionType>(hookEntry.originalFunctionAddr, hookEntry.savedOpcodeLength);

		UnProtect(hookEntry.originalFunctionAddr, hookEntry.savedOpcodeLength);
		std::memset(hookEntry.originalFunctionAddr, 0x90, hookEntry.savedOpcodeLength);

		*((unsigned char*)((unsigned int)hookEntry.originalFunctionAddr)) = 0xE9;
		*((unsigned int*)((unsigned int)hookEntry.originalFunctionAddr + 1)) = (unsigned int)hookEntry.proxyFunctionPtr - (unsigned int)((unsigned int)hookEntry.originalFunctionAddr + 5);
		ReProtect(hookEntry.originalFunctionAddr, hookEntry.savedOpcodeLength);
	}

	// ==================================================
	// Detach
	// --------------------------------------------------
	// Detaches a function detour. The original bytes are
	// restored and the tramp is then released.
	// ==================================================

	template <typename FunctionType>
	void Detach(HookEntry<FunctionType>& hookEntry)
	{
		UnProtect(hookEntry.originalFunctionAddr, hookEntry.savedOpcodeLength);
		std::memcpy((unsigned char*)hookEntry.originalFunctionAddr, (unsigned char*)*hookEntry.originalRedirectedFunctionPtr, hookEntry.savedOpcodeLength);
		ReProtect(hookEntry.originalFunctionAddr, hookEntry.savedOpcodeLength);

		ReleaseMemory((unsigned char*) *hookEntry.originalRedirectedFunctionPtr, hookEntry.savedOpcodeLength);
	}
}