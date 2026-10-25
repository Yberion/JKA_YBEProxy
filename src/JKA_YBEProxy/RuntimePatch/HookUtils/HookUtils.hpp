#pragma once

#include <cstddef>
#include <cstdint>

// ==================================================
// DetourPatcher by Deathspike, updated by Yberion
// --------------------------------------------------
// Collection of functions written to assist in detouring
// functions and patching bytes. It mainly allocates
// trampolines to execute the original bytes and uses
// jumps to go to the new function. Supports both
// Windows and Linux operating systems.
// The instructions of the target are decoded with Zydis.
// ==================================================

namespace HookUtils {
	// Size of a jmp rel32, the smallest overwrite that redirects a function
	constexpr std::size_t JumpLength = 5;
	// The instructions are copied until they cover JumpLength bytes: at most JumpLength - 1 bytes, then one instruction
	// of 15 bytes (the longest on x86)
	constexpr std::size_t MaxSavedLength = JumpLength - 1 + 15;

	enum class Status {
		Ok,
		NullTarget,
		AlreadyAttached,
		// An instruction at the target could not be decoded
		DecodeError,
		// The function ends before JumpLength bytes: the jmp would overwrite the next function
		FunctionTooShort,
		// A relative instruction other than call/jmp rel32 is in the copied bytes: it would jump to the wrong place
		// from the trampoline (on x64 this includes RIP-relative addressing)
		RelativeInstruction,
		// A rel32 does not reach its target (x64: the trampoline is more than 2 GB away)
		OutOfRange,
		AllocationFailed,
		ProtectionFailed,
		// The original bytes are back, but the memory of the trampoline could not be released
		ReleaseFailed
	};

	const char* StatusString(Status status);

	// What Attach needs to know about a hook, whatever the type of the function
	struct HookBase {
		// For the messages
		const char*		hookName;
		// exemple: Com_Printf address
		unsigned char*	originalFunctionAddr;
		// Original bytes of the target, put back by Detach
		unsigned char	savedOpcodes[MaxSavedLength];
		std::size_t		savedOpcodeLength;
		// Copy of the original bytes followed by a jmp to the rest of the function, nullptr if not attached
		unsigned char*	trampoline;

		HookBase(const char* hookName_, unsigned char* originalFunctionAddr_) :
			hookName(hookName_),
			originalFunctionAddr(originalFunctionAddr_),
			savedOpcodes{},
			savedOpcodeLength(0),
			trampoline(nullptr)
		{}
	};

	template <typename FunctionType>
	struct HookEntry : HookBase {
		// this variable will store the function ptr that will contain the trampoline address
		// exemple, it will contain the address of "void (*Original_SV_CalcPings)(void);"
		FunctionType**	originalRedirectedFunctionPtr;
		// exemple: Proxy_Com_Printf
		FunctionType*	proxyFunctionPtr;

		HookEntry(
			const char* hookName_,
			unsigned char* originalFunctionAddr_,
			FunctionType** originalRedirectedFunctionPtr_,
			FunctionType* proxyFunctionPtr_
		) :
			HookBase(hookName_, originalFunctionAddr_),
			originalRedirectedFunctionPtr(originalRedirectedFunctionPtr_),
			proxyFunctionPtr(proxyFunctionPtr_)
		{}
	};

	// Executable memory, read, write and execute. size is the same for the release
	unsigned char*	AllocateMemory(std::size_t size);
	bool			ReleaseMemory(unsigned char* address, std::size_t size);
	// Patches that don't need to be undone
	bool			Patch(unsigned char* address, unsigned char byte);
	bool			Patch_NOP_Bytes(unsigned char* address, std::size_t length);

	// Not for direct use, see Attach and Detach
	Status			AttachHook(HookBase& hook, const void* proxyFunction);
	Status			DetachHook(HookBase& hook);

	// ==================================================
	// Attach
	// --------------------------------------------------
	// Attaches a detour on the target function and makes
	// it jump to the proxy function. The instructions
	// overwritten by the jump are copied to a trampoline,
	// which ends with a jump to the rest of the function.
	// Nothing is changed unless the result is Status::Ok.
	// ==================================================

	template <typename FunctionType>
	Status Attach(HookEntry<FunctionType>& hookEntry)
	{
		const Status status = AttachHook(hookEntry, reinterpret_cast<const void*>(hookEntry.proxyFunctionPtr));

		if (status == Status::Ok)
		{
			*hookEntry.originalRedirectedFunctionPtr = reinterpret_cast<FunctionType*>(hookEntry.trampoline);
		}

		return status;
	}

	// ==================================================
	// Detach
	// --------------------------------------------------
	// Detaches a function detour. The original bytes are
	// restored and the trampoline is then released.
	// Nothing to do, and Status::Ok, if the hook is not
	// attached.
	// ==================================================

	template <typename FunctionType>
	Status Detach(HookEntry<FunctionType>& hookEntry)
	{
		const Status status = DetachHook(hookEntry);

		if (status == Status::Ok)
		{
			*hookEntry.originalRedirectedFunctionPtr = nullptr;
		}

		return status;
	}
}
