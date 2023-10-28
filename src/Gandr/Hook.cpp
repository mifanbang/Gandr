/*
 *  Gandr - another minimalism library for hacking x86-based Windows
 *  Copyright (C) 2020-2023 Mifan Bang <https://debug.tw>.
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <Hook.h>

#include <InstructionDecoder.h>
#include <Types.h>

#include <algorithm>
#include <cassert>
#include <iterator>
#include <optional>
#include <shared_mutex>
#include <span>
#include <unordered_map>
#include <vector>

#include <intrin.h>
#include <windows.h>


namespace
{


// workaround for compile error when we'd like to put Is64() in a static_assert
template <size_t>
constexpr bool k_64bitStaticAssert = gan::Is64();


struct PrologStrategy
{
	enum class Type : uint8_t
	{
		// "0xEB imm8"; relative jump; amd64 only
		// This strategy first writes an "AbsoluteJmp64" prolog to some free space within
		// a 127-byte range related to the first byte of the original prolog. Then it
		// overwrites the original prolog with a 2-byte short jump to direct to that
		// "auxiliary prolog".
		RelShortJmpToAux,

		// "0xE9 imm32"; relative jump
		// This is the only way we're using for 32-bit programs. Can also be used in
		// 64-bit programs when the hook function is close enough (<2GB in memory address).
		RelNearJmp32,

		// "mov rax, imm64" + "jmp rax"; amd64 only
		AbsoluteJmp64
	};

	Type type { };
	uint8_t imm8 { };  // optional; only valid when type=RelShortJmpToAux
};


struct Prolog
{
	constexpr static uint32_t k_maxSize = 0x18;

	uint8_t opcode[k_maxSize] { };
	uint8_t length { 0 };
};


struct Displacement32
{
	uint8_t offsetData;  // offset in prolog where a disp32 locates
	uint8_t offsetBase;  // offset in prolog from where a new disp should be calculated
	gan::ConstMemAddr targetAddr;  // absolute address to target memory address
};


struct PrologWithDisp : public Prolog
{
	std::vector<Displacement32> displacements;  // disp32 is the only supported displacement type
};


struct Trampoline
{
	constexpr static uint32_t k_size = 0x28;

	uint8_t opcode[k_size] { };
};



// ---------------------------------------------------------------------------
// Class HookRegistry: bookkeeping installed hooks. See struct
// HookRegistry::Record for what's being stored.
// ---------------------------------------------------------------------------

class HookRegistry : public gan::Singleton<HookRegistry>
{
	friend class gan::Singleton<HookRegistry>;

public:
	struct Record
	{
		Prolog original;
		Prolog modified;
		gan::MemAddr trampoline;
		PrologStrategy strategy;

		__forceinline bool IsValid() const	{ return static_cast<bool>(trampoline); }
		__forceinline operator bool() const	{ return IsValid(); }
	};

	bool Register(gan::MemAddr funcAddr, const Record& record)
	{
		std::unique_lock lock(m_mutex);

		if (!record  // Record must be valid.
			|| m_records.find(funcAddr) != m_records.end())  // Address already exists.
		{
			return false;
		}
		m_records.emplace(funcAddr, record);
		return true;
	}

	gan::ConstMemAddr GetTrampoline(gan::MemAddr funcAddr) const
	{
		std::shared_lock lock(m_mutex);

		const auto record = LookUp(funcAddr);
		return record ?
			record->trampoline.Immutable() :
			gan::ConstMemAddr{ nullptr };
	}

	std::optional<Record> LookUp(const gan::MemAddr funcAddr) const
	{
		std::shared_lock lock(m_mutex);

		const auto& itr = m_records.find(funcAddr);
		return itr != m_records.end() ?
			std::make_optional<Record>(itr->second) :
			std::nullopt;
	}

	std::optional<Record> Unregister(const gan::MemAddr funcAddr)
	{
		std::unique_lock lock(m_mutex);

		const auto& itr = m_records.find(funcAddr);
		if (itr != m_records.end())
		{
			auto unregisteredRecord = itr->second;
			m_records.erase(itr);
			return unregisteredRecord;
		}
		return std::nullopt;
	}

private:
	HookRegistry() = default;

	std::unordered_map<gan::MemAddr, Record> m_records;

	// Although Gandr doesn't support multi-threaded hook installation/uninstallation,
	// it still needs to support situations where one thread is calling a hooked function
	// and potentially looking up address to its trampoline while another thread making
	// modifications to the hook registry.
	mutable std::shared_mutex m_mutex;
};


// ---------------------------------------------------------------------------
// Class TrampolineRegistry: managing memory pages for trampoline storage
// ---------------------------------------------------------------------------

class TrampolineRegistry : public gan::Singleton<TrampolineRegistry>
{
	friend class gan::Singleton<TrampolineRegistry>;

public:
	constexpr static auto k_trampolineSize = Trampoline::k_size;

	// Try allocating a trampoline within the range of 32-bit offset from "desiredAddress".
	gan::MemAddr Register(const Trampoline& trampoline, gan::MemRange desiredAddrRange)
	{
		std::unique_lock lock(m_mutex);

		const auto pageIndex =
			FindRelevantPageInRange(desiredAddrRange)
			.value_or(
				AddNewPage(desiredAddrRange)
			);
		assert(m_freeLists[pageIndex].size() > 0);

		// get a free slot
		FreeSlot slot = m_freeLists[pageIndex].back();
		m_freeLists[pageIndex].pop_back();

		auto trampolineAddr = m_pages[pageIndex].Offset(slot.pageOffset);
		memcpy(trampolineAddr.Ptr<uint8_t>(), trampoline.opcode, k_trampolineSize);

		assert(m_records.find(trampolineAddr) == m_records.end());
		m_records.try_emplace(trampolineAddr, pageIndex);

		return trampolineAddr;
	}

	void Unregister(gan::MemAddr addr)
	{
		std::unique_lock lock(m_mutex);

		const auto& itr = m_records.find(addr);
		if (itr == m_records.end())
			return;

		const auto pageIndex = itr->second;
		const auto offset = static_cast<unsigned int>(addr - m_pages[pageIndex]);  // negative offset will be treated as a big, positve offset
		assert(offset <= m_allocGranularity - k_trampolineSize);
		m_freeLists[pageIndex].emplace_back(offset);
		m_records.erase(itr);
	}

private:
	TrampolineRegistry()
		: m_records()
		, m_pages()
		, m_freeLists()
		, m_allocGranularity(GetAllocGranularity())
		, m_mutex()
	{ }

	static int GetAllocGranularity()
	{
		SYSTEM_INFO sysInfo;
		::GetSystemInfo(&sysInfo);
		return sysInfo.dwAllocationGranularity;
	}

	// Assumes that "desiredAddrRange" is aligned with the result from GetAllocGranularity()
	static gan::MemAddr FindPageForAlloc(gan::MemRange desiredAddrRange)
	{
		if constexpr (gan::Is64())
		{
			const auto allocGranularity = GetAllocGranularity();
			const auto addrToFindStart = desiredAddrRange.min;
			const auto addrToFindEnd = desiredAddrRange.max.Offset(-allocGranularity);

			// Iterate through pages until hitting one with the MEM_FREE state.
			MEMORY_BASIC_INFORMATION memInfo;
			for (auto addr = addrToFindStart; addr < addrToFindEnd; )
			{
				const auto result = ::VirtualQuery(addr.ConstPtr<uint8_t>(), &memInfo, sizeof(memInfo));
				if (result == 0)
				{
					// VirtualQuery failed with the address passed in
					addr = addr.Offset(allocGranularity);
					continue;
				}

				if (memInfo.State == MEM_FREE)
				{
					return addr;
				}
				else
				{
					const auto offset = AlignMemAddrWithGranularity(memInfo.RegionSize, allocGranularity);
					addr = gan::MemAddr{ memInfo.BaseAddress }.Offset(offset);
				}
			}
			return gan::MemAddr{ nullptr };  // Not found
		}
		else
			return gan::MemAddr{ nullptr };  // Don't care in 32 bit
	}

	static size_t GenerateMaskFromGranularity(unsigned int granularity)
	{
		const auto tzcnt = _tzcnt_u32(granularity);
		assert(_lzcnt_u32(granularity) + tzcnt == 31);  // "granularity" must a power of 2.
		return std::numeric_limits<size_t>::max() << tzcnt;
	}

	static size_t AlignMemAddrWithGranularity(const size_t addr, unsigned int granularity)
	{
		const auto offsetMask = GenerateMaskFromGranularity(granularity);
		return (addr & offsetMask) + (addr & ~offsetMask ? granularity : 0);
	}

	static gan::MemRange AlignMemRangeWithGranularity(gan::MemRange memRange, unsigned int granularity)
	{
		auto offsetMask = GenerateMaskFromGranularity(granularity);
		const auto rangeMin = memRange.min;
		const auto rangeMax = memRange.max;
		return {
			.min = (rangeMin & offsetMask).Offset(rangeMin & ~offsetMask ? granularity : 0),
			.max = (rangeMax & offsetMask)  // Not to return an address higher than memRange.max
		};
	}

	// Assumes that caller has already obtained a lock
	std::optional<unsigned int> FindRelevantPageInRange(gan::MemRange desiredAddrRange) const
	{
		for (unsigned int i = 0; i < static_cast<unsigned int>(m_pages.size()); ++i)
		{
			if (m_freeLists[i].empty())
				continue;

			if constexpr (gan::Is64())
			{
				if (desiredAddrRange.InRange(m_pages[i]))
					return i;
			}
			else
				return i;
		}
		return std::nullopt;
	}

	// Assume: caller has already obtained a lock
	unsigned int AddNewPage(gan::MemRange desiredAddrRange)
	{
		const gan::MemRange fixedAddrRange = AlignMemRangeWithGranularity(desiredAddrRange, m_allocGranularity);

		auto desiredAddress{ FindPageForAlloc(fixedAddrRange) };
		gan::MemAddr newPageAddr{ ::VirtualAlloc(
			desiredAddress.Ptr<uint8_t>(),
			m_allocGranularity,
			MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE
		) };
		assert(newPageAddr);

		m_pages.emplace_back(newPageAddr);
		m_freeLists.emplace_back();

		// Generate free list
		const unsigned int lastPageIndex = static_cast<unsigned int>(m_pages.size()) - 1;
		const unsigned int numTrampolinesPerPage = m_allocGranularity / k_trampolineSize;
		for (unsigned int i = 0, offset = 0; i < numTrampolinesPerPage; ++i, offset += k_trampolineSize)
			m_freeLists[lastPageIndex].emplace_back(FreeSlot { offset });

		return lastPageIndex;
	}


	struct FreeSlot
	{
		unsigned int pageOffset;
	};
	using FreeList = std::vector<FreeSlot>;

	// Each page in "m_pages" is of the size of "m_allocGranularity" bytes
	std::unordered_map<gan::MemAddr, size_t> m_records;  // Mapping address to page index
	std::vector<gan::MemAddr> m_pages;  // Base addresses of pages
	std::vector<FreeList> m_freeLists;  // Shared index with "m_pages"

	unsigned int m_allocGranularity;

	mutable std::shared_mutex m_mutex;  // For the reason of using mutex, see HookRegistry::m_mutex.
};


// ---------------------------------------------------------------------------
// Class OpcodeGenerator: filling in instructions to buffer; might be used
//                        by both hooks and trampolines.
// ---------------------------------------------------------------------------

class OpcodeGenerator
{
public:
	template <uint8_t Length>
	class Base
	{
	public:
		constexpr static uint8_t k_length = Length;
	};

	// This jump modifies rax and is recommended to only be used for hooks.
	class AbsLongJmpRax : public Base<12>
	{
	public:
		template <size_t N>
		static uint8_t Make(gan::MemAddr targetAddr, uint8_t(&out)[N])
		{
			static_assert(k_64bitStaticAssert<N>);
			static_assert(N >= k_length);

			// mov rax, imm64
			out[0] = 0x48;  // REX.W
			out[1] = 0xB8;  // mov
			gan::MemAddr{ out + 2 }.Ref<gan::MemAddr>() = targetAddr;

			// jmp rax
			out[10] = 0xFF;
			out[11] = 0xE0;  // mod=11b, reg=4, r/m=0

			return k_length;
		}
	};

	// This jump is longer in length but doesn't have side effects. Trampolines are recommended to use this.
	class AbsLongJmp64 : public Base<14>
	{
	public:
		template <size_t N>
		static uint8_t Make(gan::MemAddr targetAddr, uint8_t(&out)[N])
		{
			static_assert(k_64bitStaticAssert<N>);
			static_assert(N >= k_length);

			const auto addr64 = reinterpret_cast<uint64_t>(targetAddr.ConstPtr());
			const auto addrLow = static_cast<uint32_t>(addr64);
			const auto addrHigh = static_cast<uint32_t>(addr64 >> 32ull);

			// push imm32(lower 32 bits of addr)
			out[0] = 0x68;  // push
			*reinterpret_cast<uint32_t*>(out + 1) = addrLow;

			// mov dword ptr [rsp+4], imm32(higher 32 bits of addr)
			out[5] = 0xC7;  // mov /0
			out[6] = 0x44;  // mod=01b, reg=0, r/m=100b
			out[7] = 0x24;  // ss=00b, index=100b, base=100b
			out[8] = 0x04;  // disp8
			*reinterpret_cast<uint32_t*>(out + 9) = addrHigh;

			out[13] = 0xC3;  // ret

			return k_length;
		}
	};

	// Used by 32-bit trampolines
	class AbsLongJmp32 : public Base<6>
	{
	public:
		template <size_t N>
		static uint8_t Make(gan::MemAddr targetAddr, uint8_t(&out)[N])
		{
			static_assert(!k_64bitStaticAssert<N>);
			static_assert(N >= k_length);

			// push imm32
			out[0] = 0x68;  // push
			gan::MemAddr{ out + 1 }.Ref<gan::MemAddr>() = targetAddr;

			// ret
			out[5] = 0xC3;

			return k_length;
		}
	};

	class RelNearJmp32 : public Base<5>
	{
	public:
		template <size_t N>
		static uint8_t Make(gan::MemAddr originAddr, gan::MemAddr targetAddr, uint8_t(&out)[N])
		{
			static_assert(N >= k_length);

			// jmp imm32
			out[0] = 0xE9;  // near jmp
			*reinterpret_cast<int32_t*>(out + 1) = static_cast<int32_t>(
				targetAddr - originAddr - k_length
			);
			return k_length;
		}
	};

	class RelShortJmp8 : public Base<2>
	{
	public:
		template <size_t N>
		static uint8_t Make(gan::MemAddr originAddr, int8_t offset, uint8_t(&out)[N])
		{
			static_assert(N >= k_length);

			out[0] = 0xEB;  // near jmp
			out[1] = offset;
			return k_length;
		}
	};
};


PrologStrategy DetermineStrategy(gan::MemAddr origFunc, gan::MemAddr hookFunc)
{
	constexpr uint8_t k_lenRelShortJmpToAux = OpcodeGenerator::RelShortJmp8::k_length;
	constexpr uint8_t k_lenRelNearJmp32 = OpcodeGenerator::RelNearJmp32::k_length;
	constexpr uint8_t k_lenAbsoluteJmpRax = OpcodeGenerator::AbsLongJmpRax::k_length;

	constexpr PrologStrategy k_RelNearJmp32 { PrologStrategy::Type::RelNearJmp32 };
	constexpr PrologStrategy k_AbsoluteJmp64 { PrologStrategy::Type::AbsoluteJmp64 };

	if constexpr (gan::Is64())
	{
		const auto addrDiff = origFunc > hookFunc ?
			origFunc - hookFunc :
			hookFunc - origFunc;

		if (addrDiff < 0x7FFF'FFFFull - k_lenRelNearJmp32)  // disp is signed
			return k_RelNearJmp32;  // Addressable with 32-bit displacement

		for (intptr_t i = k_lenRelShortJmpToAux; i < 127 + k_lenRelShortJmpToAux - k_lenAbsoluteJmpRax; ++i)
		{
			auto start = origFunc.Offset(i).ConstPtr<uint8_t>();
			auto end = origFunc.Offset(i + k_lenAbsoluteJmpRax).ConstPtr<uint8_t>();
			if (std::all_of(start, end, [](uint8_t c) { return c == 0xCC; } ))
			{
				return {
					PrologStrategy::Type::RelShortJmpToAux,
					static_cast<uint8_t>(i - k_lenRelShortJmpToAux)
				};
			}
		}

		return k_AbsoluteJmp64;  // worst case scenario
	}
	else
		return k_RelNearJmp32;
}


Prolog GenerateHookProlog(gan::MemAddr origFunc, gan::MemAddr hookFunc, PrologStrategy strategy)
{
	Prolog result;
	if constexpr (gan::Is64())
	{
		if (strategy.type == PrologStrategy::Type::RelShortJmpToAux)
			result.length = OpcodeGenerator::RelShortJmp8::Make(hookFunc, strategy.imm8, result.opcode);

		else if (strategy.type == PrologStrategy::Type::RelNearJmp32)
			result.length = OpcodeGenerator::RelNearJmp32::Make(origFunc, hookFunc, result.opcode);

		else if (strategy.type == PrologStrategy::Type::AbsoluteJmp64)
			result.length = OpcodeGenerator::AbsLongJmpRax::Make(hookFunc, result.opcode);
	}
	else
		result.length = OpcodeGenerator::RelNearJmp32::Make(origFunc, hookFunc, result.opcode);

	assert(result.length > 0);
	return result;
}


bool WriteMemory(gan::MemAddr address, const std::span<const uint8_t>& data)
{
	DWORD oldAttr, dummy;
	auto rawPtr = address.Ptr<uint8_t>();
	::VirtualProtect(rawPtr, data.size(), PAGE_EXECUTE_READWRITE, &oldAttr);
	memcpy(rawPtr, data.data(), data.size());
	::VirtualProtect(rawPtr, data.size(), oldAttr, &dummy);
	return true;
}


// ---------------------------------------------------------------------------
// Class AuxiliaryPrologHelper: Helper functions to handle the creation and
// deletion of an auxiliary prolog when the strategy is RelShortJmpToAux.
// ---------------------------------------------------------------------------

class AuxiliaryPrologHelper
{
public:
	constexpr static bool ShouldUseAuxProlog(PrologStrategy::Type strategyType)
	{
		if constexpr (gan::Is64())
			return strategyType == PrologStrategy::Type::RelShortJmpToAux;
		else
			return false;
	}

	static bool Create(gan::MemAddr origFunc, gan::MemAddr hookFunc, uint8_t offsetToAux)
	{
		constexpr PrologStrategy k_strategyAbsLongJmp { PrologStrategy::Type::AbsoluteJmp64 };
		const auto mainHookProlog = GenerateHookProlog(origFunc, hookFunc, k_strategyAbsLongJmp);

		return WriteMemory(
			origFunc.Offset(OpcodeGenerator::RelShortJmp8::k_length).Offset(offsetToAux),
			std::span(mainHookProlog.opcode, mainHookProlog.length)
		);
	}

	static bool Delete(gan::MemAddr origFunc, uint8_t offsetToAux)
	{
		static_assert(OpcodeGenerator::AbsLongJmpRax::k_length == 12);
		const static uint8_t k_int3Opcodes[OpcodeGenerator::AbsLongJmpRax::k_length] {
			0xCC, 0xCC, 0xCC, 0xCC,
			0xCC, 0xCC, 0xCC, 0xCC,
			0xCC, 0xCC, 0xCC, 0xCC
		};
		return WriteMemory(
			origFunc.Offset(OpcodeGenerator::RelShortJmp8::k_length).Offset(offsetToAux),
			std::span(k_int3Opcodes)
		);
	}
};


std::optional<PrologWithDisp> CopyProlog(gan::ConstMemAddr addr, uint8_t length)
{
	PrologWithDisp copiedProlog;

	gan::InstructionDecoder decoder(addr);
	while (copiedProlog.length < length)
	{
		if (const auto nextInstInfo = decoder.GetNextLength())
		{
			const uint8_t nextInstLen = nextInstInfo->GetLength();

			if (copiedProlog.length + nextInstLen <= Prolog::k_maxSize)  // Check remaining space for the instruction
			{
				memcpy(
					copiedProlog.opcode + copiedProlog.length,
					addr.Offset(copiedProlog.length).ConstPtr<uint8_t>(),
					nextInstLen
				);
				copiedProlog.length += nextInstLen;

				// Record a displacement if base is EIP/RIP
				if (nextInstInfo->dispNeedsFixup)
				{
					// Disp and imm are the last two parts of an instruction
					const uint8_t offsetData = copiedProlog.length - nextInstInfo->lengthImm - nextInstInfo->lengthDisp;

					if (nextInstInfo->lengthDisp == 4)
					{
						const auto disp32 = addr.Offset(offsetData).ConstRef<int32_t>();  // Disp32 field in instruction
						const Displacement32 newDispEntry{
							.offsetData = offsetData,
							.offsetBase = copiedProlog.length,
							.targetAddr = addr.Offset(copiedProlog.length).Offset(disp32)
						};
						copiedProlog.displacements.emplace_back(newDispEntry);
					}
					else
					{
						// Normally only a 4-byte displacement is fixable and thus accepted in prolog, but in rare instances
						// where an 1-byte disp points to an address inside the prolog, it's safe to just copy as-is.
						if (nextInstInfo->lengthDisp != 1
							|| copiedProlog.length + addr.Offset(offsetData).ConstRef<int8_t>() >= length)
						{
							return std::nullopt;
						}
					}

				}

				continue;  // Only continue decoding the next instruction if we get here successfully
			}
		}

		return std::nullopt;
	}

	return copiedProlog;
}


gan::MemRange GetAddressableRange(gan::MemAddr tramAddr, const std::vector<Displacement32>& displacements)
{
	if constexpr (gan::Is64())
	{
		// Combine "tramAddr" and "displacements" into a uniformly-typed vector.
		std::vector<gan::MemAddr> addresses;
		addresses.reserve(displacements.size() + 1);
		addresses.emplace_back(tramAddr);
		std::transform(
			displacements.begin(),
			displacements.end(),
			std::back_inserter(addresses),
			[](const Displacement32& disp) {
				return disp.targetAddr.ConstCast();
			}
		);

		// Get max and min.
		auto [itrMin, itrMax] = std::minmax_element(addresses.begin(), addresses.end(), std::less<gan::MemAddr>{ });
		assert(itrMin != addresses.end());
		assert(itrMax != addresses.end());

		// min and max must be within the range addressable by disp32, and since
		// the MSB in disp32 is a sign bit, their distance can't go beyond 0x7FFF'FFFF.
		gan::MemRange addrRange{
			.min = itrMax->Offset(-0x7FFF'0000ll),  // min must be addressable by disp32 from itrMax,
			.max = itrMin->Offset(0x7FFF'0000ll)    // and the same for max.
		};
		assert(addrRange.max > addrRange.min);
		return addrRange;
	}
	else
	{
		// Easy enough for 32-bit system.
		return {
			gan::MemAddr{ reinterpret_cast<void*>(0x1'0000u) },
			gan::MemAddr{ reinterpret_cast<void*>(0x7FFF'0000u) }
		};
	}
}


void FixupDisplacements(gan::MemAddr trampolineAddr, const std::vector<Displacement32>& displacements)
{
	for (const Displacement32& disp : displacements)
	{
		const auto newDisplacement = static_cast<uint32_t>(
			disp.targetAddr - trampolineAddr.Offset(disp.offsetBase).Immutable()
		);
		trampolineAddr.Offset(disp.offsetData).Ref<uint32_t>() = newDisplacement;
	}
}


Trampoline GenerateTrampoline(gan::MemAddr origFuncAddr, const Prolog& prolog)
{
	// The longest jump across platforms is 14-byte long. See OpcodeGenerator::AbsLongJmp64.
	static_assert(Trampoline::k_size >= Prolog::k_maxSize + OpcodeGenerator::AbsLongJmp64::k_length);

	Trampoline result;
	memcpy(result.opcode, prolog.opcode, prolog.length);

	if constexpr (gan::Is64())
	{
		auto& opcodeJmp = reinterpret_cast<uint8_t(&)[14]>(result.opcode[prolog.length]);  // ugly...
		OpcodeGenerator::AbsLongJmp64::Make(origFuncAddr.Offset(prolog.length), opcodeJmp);
	}
	else
	{
		// Instruction length is less of an issue for trampolines as we usually have plenty of space.
		// Using a 6-byte "push and ret" instead of a 5-byte relative jump is fine.
		auto& opcodeJmp = reinterpret_cast<uint8_t(&)[6]>(result.opcode[prolog.length]);  // ugly...
		OpcodeGenerator::AbsLongJmp32::Make(origFuncAddr.Offset(prolog.length), opcodeJmp);
	}
	return result;
}


}  // unnamed namespace



namespace gan
{


// ---------------------------------------------------------------------------
// Class Hook
// ---------------------------------------------------------------------------

Hook::OpResult Hook::Install()
{
	if (m_hooked)
		return OpResult::Hooked;

	HookRegistry& hookReg = HookRegistry::GetInstance();
	if (hookReg.GetTrampoline(m_funcOrig).IsValid())
		return OpResult::AddressInUse;

	// Determine what should be written to the target function.
	auto strategy = DetermineStrategy(m_funcOrig, m_funcHook);
	if (AuxiliaryPrologHelper::ShouldUseAuxProlog(strategy.type)
		&& !AuxiliaryPrologHelper::Create(m_funcOrig, m_funcHook, strategy.imm8))
	{
		strategy.type = PrologStrategy::Type::AbsoluteJmp64;  // Fall back to writing the absolute jump in place.
	}

	// Generate a new prolog and backup the original one.
	const auto hookProlog = GenerateHookProlog(m_funcOrig, m_funcHook, strategy);
	const auto origProlog = CopyProlog(m_funcOrig.Immutable(), hookProlog.length);
	if (!origProlog)
		return OpResult::PrologNotSupported;

	// Trampoline to get back to the original function body
	const auto trampoline = GenerateTrampoline(m_funcOrig, *origProlog);

	// Find the address range in which all displacements of the original prolog are addressable.
	// Address of the original function is also taken into consideration.
	const auto desiredTramAddrRange = GetAddressableRange(m_funcOrig, origProlog->displacements);

	// Allocate memory and fix up displacements for trampoline.
	TrampolineRegistry& trampolineReg = TrampolineRegistry::GetInstance();
	auto trampolineAddr = trampolineReg.Register(trampoline, desiredTramAddrRange);
	if (!trampolineAddr)
		return OpResult::TrampolineAllocFailed;
	if (origProlog->displacements.size() > 0)
		FixupDisplacements(trampolineAddr, origProlog->displacements);

	// Register hook and modify memory
	HookRegistry::Record newHookRecord {
		.original = *origProlog,
		.modified = hookProlog,
		.trampoline = trampolineAddr,
		.strategy = strategy
	};
	if (hookReg.Register(m_funcOrig, newHookRecord))
	{
		if (WriteMemory(m_funcOrig, std::span(hookProlog.opcode, hookProlog.length)))
		{
			m_hooked = true;
			return OpResult::Hooked;
		}
		else
			hookReg.Unregister(m_funcOrig);
	}

	trampolineReg.Unregister(trampolineAddr);
	if (AuxiliaryPrologHelper::ShouldUseAuxProlog(strategy.type))
		AuxiliaryPrologHelper::Delete(m_funcOrig, strategy.imm8);
	return OpResult::AccessDenied;
}


Hook::OpResult Hook::Uninstall()
{
	if (!m_hooked)
		return OpResult::NotHooked;

	HookRegistry& hookReg = HookRegistry::GetInstance();
	if (const auto record = hookReg.LookUp(m_funcOrig))
	{
		const Prolog& expectedHookProlog = record->modified;

		// Make sure the prolog altered by our hook hasn't been modified by others.
		if (memcmp(m_funcOrig.Ptr<uint8_t>(), expectedHookProlog.opcode, expectedHookProlog.length) != 0)
			return OpResult::PrologMismatched;

		// We don't expect user making hook modification across threads. Thus no check
		// is made against the results from LookUp() and Unregister().
		hookReg.Unregister(m_funcOrig);

		const Prolog& origProlog = record->original;
		if (WriteMemory(m_funcOrig, std::span(origProlog.opcode, origProlog.length)))
			TrampolineRegistry::GetInstance().Unregister(record->trampoline);
		else
		{
			// Failed to write memory somehow. Have to restore hook registry.
			hookReg.Register(m_funcOrig, *record);
			return OpResult::AccessDenied;
		}

		if (AuxiliaryPrologHelper::ShouldUseAuxProlog(record->strategy.type))
			AuxiliaryPrologHelper::Delete(m_funcOrig, record->strategy.imm8);

		m_hooked = false;
		return OpResult::Unhooked;
	}

	m_hooked = false;
	return OpResult::NotHooked;
}


void Hook::AssertCtorArgs(MemAddr origFunc, MemAddr hookFunc)
{
	assert(origFunc);
	assert(hookFunc);
	assert(origFunc != hookFunc);
}


ConstMemAddr Hook::GetTrampolineAddr(ConstMemAddr origFunc)
{
	const auto addr = HookRegistry::GetInstance().GetTrampoline(origFunc.ConstCast());
	assert(addr);
	return addr;
}


}  // namespace gan
