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

#ifndef WIN32
	#include <sys/mman.h>
	#include <unistd.h>
#else
	#include <windows.h>
#endif

#include <cstring>
#include <Zydis/Zydis.h>

#include "JKA_YBEProxy/HookUtils/HookUtils.hpp"

namespace HookUtils {
	namespace {
		// Makes code writable while the object exists, then puts its protection back and flushes the instruction cache
		// Only for code: the protection put back is read and execute
		class CodeWriter {
		public:
			CodeWriter(void* codeAddress, std::size_t codeLength) :
				address(codeAddress),
				length(codeLength),
				writable(false)
			{
#ifndef WIN32
				const uintptr_t pageSize = (uintptr_t)sysconf(_SC_PAGESIZE);

				pageStart = (uintptr_t)address & ~(pageSize - 1);
				pageLength = (((uintptr_t)address + length + pageSize - 1) & ~(pageSize - 1)) - pageStart;
				writable = mprotect((void*)pageStart, pageLength, PROT_READ | PROT_WRITE | PROT_EXEC) == 0;
#else
				writable = VirtualProtect(address, length, PAGE_EXECUTE_READWRITE, &oldProtection) != 0;
#endif
			}

			~CodeWriter()
			{
				if (!writable)
				{
					return;
				}

#ifndef WIN32
				// Nothing to do on x86, the compiler builtin documents the intent
				__builtin___clear_cache((char*)address, (char*)address + length);
				mprotect((void*)pageStart, pageLength, PROT_READ | PROT_EXEC);
#else
				DWORD dummy;

				FlushInstructionCache(GetCurrentProcess(), address, length);
				VirtualProtect(address, length, oldProtection, &dummy);
#endif
			}

			CodeWriter(const CodeWriter&) = delete;
			CodeWriter& operator=(const CodeWriter&) = delete;

			bool IsWritable() const { return writable; }

		private:
			void*		address;
			std::size_t	length;
			bool		writable;
#ifndef WIN32
			uintptr_t	pageStart = 0;
			std::size_t	pageLength = 0;
#else
			DWORD		oldProtection = 0;
#endif
		};

		// Offset to put in a jmp/call rel32 that ends at nextInstruction to reach destination
		// On x86 a rel32 reaches every address (the offset wraps around), on x64 it only reaches +-2 GB
		bool Rel32(const void* nextInstruction, const void* destination, int32_t& offset)
		{
			const uintptr_t difference = (uintptr_t)destination - (uintptr_t)nextInstruction;

			if constexpr (sizeof(void*) == 4)
			{
				offset = (int32_t)difference;
				return true;
			}
			else
			{
				const int64_t signedDifference = (int64_t)difference;

				if (signedDifference < INT32_MIN || signedDifference > INT32_MAX)
				{
					return false;
				}

				offset = (int32_t)signedDifference;
				return true;
			}
		}

		// ==================================================
		// Analyze
		// --------------------------------------------------
		// Decodes the instructions at the target until they
		// cover JumpLength bytes, and checks that they can
		// run from a trampoline. Gives their total length,
		// and where the call/jmp rel32 to relocate is, if any.
		// A relative instruction is only safe when its offset
		// is rewritten for its new place. A rel32 call/jmp
		// is, any other (jcc, short jmp, loop, RIP-relative
		// addressing on x64) is refused.
		// ==================================================

		Status Analyze(const unsigned char* target, std::size_t& length, bool& hasRelocation, std::size_t& relocationOffset)
		{
			constexpr bool is32Bit = sizeof(void*) == 4;
			ZydisDecoder decoder;

			if (!ZYAN_SUCCESS(ZydisDecoderInit(&decoder, is32Bit ? ZYDIS_MACHINE_MODE_LEGACY_32 : ZYDIS_MACHINE_MODE_LONG_64,
				is32Bit ? ZYDIS_STACK_WIDTH_32 : ZYDIS_STACK_WIDTH_64)))
			{
				return Status::DecodeError;
			}

			length = 0;
			hasRelocation = false;
			relocationOffset = 0;

			while (length < JumpLength)
			{
				ZydisDecodedInstruction instruction;
				ZydisDecodedOperand operands[ZYDIS_MAX_OPERAND_COUNT];

				if (!ZYAN_SUCCESS(ZydisDecoderDecodeFull(&decoder, target + length, ZYDIS_MAX_INSTRUCTION_LENGTH, &instruction, operands)))
				{
					return Status::DecodeError;
				}

				const std::size_t nextLength = length + instruction.length;

				if (instruction.attributes & ZYDIS_ATTRIB_IS_RELATIVE)
				{
					// E8 (call) and E9 (jmp) with a 32-bit offset, and no prefix
					const bool isRel32CallOrJmp =
						instruction.length == JumpLength
						&& instruction.opcode_map == ZYDIS_OPCODE_MAP_DEFAULT
						&& (instruction.opcode == 0xE8 || instruction.opcode == 0xE9)
						&& instruction.raw.imm[0].is_relative
						&& instruction.raw.imm[0].size == 32;

					if (!isRel32CallOrJmp)
					{
						return Status::RelativeInstruction;
					}

					// Such an instruction is JumpLength bytes long: the loop ends right after it, there is only one
					hasRelocation = true;
					relocationOffset = length;
				}
				else if (nextLength < JumpLength && instruction.meta.category == ZYDIS_CATEGORY_RET)
				{
					return Status::FunctionTooShort;
				}

				length = nextLength;
			}

			return Status::Ok;
		}
	}

	const char* StatusString(Status status)
	{
		switch (status)
		{
		case Status::Ok:
			return "no error";
		case Status::NullTarget:
			return "no address for the function or for its replacement";
		case Status::AlreadyAttached:
			return "already attached";
		case Status::DecodeError:
			return "could not decode the instructions of the function";
		case Status::FunctionTooShort:
			return "the function is shorter than the 5 bytes of the jump";
		case Status::RelativeInstruction:
			return "the first instructions contain a relative instruction that cannot be moved to the trampoline";
		case Status::OutOfRange:
			return "a 32-bit offset does not reach its target";
		case Status::AllocationFailed:
			return "could not allocate memory for the trampoline";
		case Status::ProtectionFailed:
			return "could not make the code writable";
		case Status::ReleaseFailed:
			return "could not release the memory of the trampoline";
		}

		return "unknown error";
	}

	// ==================================================
	// AllocateMemory
	// --------------------------------------------------
	// Use OS specific way of allocating memory when available
	// ==================================================

	unsigned char* AllocateMemory(std::size_t size)
	{
#ifndef WIN32
		void* allocAddr = mmap(nullptr, size, PROT_EXEC | PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

		if (allocAddr == MAP_FAILED)
		{
			return nullptr;
		}

		return (unsigned char*)allocAddr;
#else
		return (unsigned char*)VirtualAlloc(nullptr, size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
#endif
	}

	// ==================================================
	// ReleaseMemory
	// --------------------------------------------------
	// Use OS specific way of releasing memory when available
	// ==================================================

	bool ReleaseMemory(unsigned char* address, std::size_t size)
	{
#ifndef WIN32
		return munmap(address, size) == 0;
#else
		// VirtualFree with MEM_RELEASE needs a size of 0: the whole allocation is released
		(void)size;

		return VirtualFree(address, 0, MEM_RELEASE) != 0;
#endif
	}

	// ==================================================
	// Patch
	// --------------------------------------------------
	// Patches a single byte at the target address,
	// usually performed to skip instructions from that
	// given address.
	// ==================================================

	bool Patch(unsigned char* address, unsigned char byte)
	{
		CodeWriter writer(address, 1);

		if (!writer.IsWritable())
		{
			return false;
		}

		*address = byte;

		return true;
	}

	// ==================================================
	// Patch_NOP_Bytes
	// --------------------------------------------------
	// Patches X bytes to NOP at the target address,
	// usually performed to skip instructions from that
	// given address.
	// ==================================================

	bool Patch_NOP_Bytes(unsigned char* address, std::size_t length)
	{
		CodeWriter writer(address, length);

		if (!writer.IsWritable())
		{
			return false;
		}

		std::memset(address, 0x90, length);

		return true;
	}

	// ==================================================
	// AttachHook
	// --------------------------------------------------
	// Everything that can fail is checked before the
	// target is touched: after a result other than Ok, the
	// function is as it was, and nothing stays allocated.
	// ==================================================

	Status AttachHook(HookBase& hook, const void* proxyFunction)
	{
		unsigned char* const target = hook.originalFunctionAddr;

		if (!target || !proxyFunction)
		{
			return Status::NullTarget;
		}

		if (hook.trampoline)
		{
			return Status::AlreadyAttached;
		}

		std::size_t length;
		bool hasRelocation;
		std::size_t relocationOffset;
		const Status analysis = Analyze(target, length, hasRelocation, relocationOffset);

		if (analysis != Status::Ok)
		{
			return analysis;
		}

		int32_t jumpToProxy;

		if (!Rel32(target + JumpLength, proxyFunction, jumpToProxy))
		{
			return Status::OutOfRange;
		}

		unsigned char* const trampoline = AllocateMemory(length + JumpLength);

		if (!trampoline)
		{
			return Status::AllocationFailed;
		}

		std::memcpy(trampoline, target, length);

		// jmp to the rest of the function
		int32_t jumpBack;
		bool inRange = Rel32(trampoline + length + JumpLength, target + length, jumpBack);

		if (inRange && hasRelocation)
		{
			// The copied call/jmp now sits at another address: change its offset so that it reaches the same destination
			int32_t offset;

			std::memcpy(&offset, target + relocationOffset + 1, sizeof(offset));

			const uintptr_t destination = (uintptr_t)(target + relocationOffset + JumpLength) + (uintptr_t)(intptr_t)offset;

			inRange = Rel32(trampoline + relocationOffset + JumpLength, (const void*)destination, offset);

			if (inRange)
			{
				std::memcpy(trampoline + relocationOffset + 1, &offset, sizeof(offset));
			}
		}

		if (!inRange)
		{
			ReleaseMemory(trampoline, length + JumpLength);

			return Status::OutOfRange;
		}

		trampoline[length] = 0xE9;
		std::memcpy(trampoline + length + 1, &jumpBack, sizeof(jumpBack));

		{
			CodeWriter writer(target, length);

			if (!writer.IsWritable())
			{
				ReleaseMemory(trampoline, length + JumpLength);

				return Status::ProtectionFailed;
			}

			std::memcpy(hook.savedOpcodes, target, length);
			std::memset(target, 0x90, length);
			target[0] = 0xE9;
			std::memcpy(target + 1, &jumpToProxy, sizeof(jumpToProxy));
		}

		hook.savedOpcodeLength = length;
		hook.trampoline = trampoline;

		return Status::Ok;
	}

	// ==================================================
	// DetachHook
	// --------------------------------------------------
	// The original bytes come from the copy made by
	// AttachHook, not from the trampoline: a relocated
	// offset there is not the engine's.
	// ==================================================

	Status DetachHook(HookBase& hook)
	{
		if (!hook.trampoline)
		{
			return Status::Ok;
		}

		{
			CodeWriter writer(hook.originalFunctionAddr, hook.savedOpcodeLength);

			if (!writer.IsWritable())
			{
				return Status::ProtectionFailed;
			}

			std::memcpy(hook.originalFunctionAddr, hook.savedOpcodes, hook.savedOpcodeLength);
		}

		const bool released = ReleaseMemory(hook.trampoline, hook.savedOpcodeLength + JumpLength);

		hook.trampoline = nullptr;
		hook.savedOpcodeLength = 0;

		return released ? Status::Ok : Status::ReleaseFailed;
	}
}
