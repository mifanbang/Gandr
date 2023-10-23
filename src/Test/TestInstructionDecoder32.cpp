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

#include "Test.h"

#include <InstructionDecoder.h>


DEFINE_TESTSUITE_START(InstructionDecoder_IA32)

	// ---------------------------------------------------------------------------
	// IA-32-only
	// ---------------------------------------------------------------------------

	// Reusing instruction from 64-bit version of "MovzxFromDisp32"
	DEFINE_TEST_START(DecodeRexPrefixAsOpcode)
	{
		// 64-bit mode: REX.W movzx  rax, byte ptr [74172A1A21h]
		// 32-bit mode: dec  eax
		//				movzx  eax, byte ptr ds:[44332211h]
		const static uint8_t k_inRexMovzxFromDisp32[] { 0x48, 0x0F, 0xB6, 0x05, 0x11, 0x22, 0x33, 0x44 };
		gan::InstructionDecoder decoder(gan::Arch::IA32, gan::ConstMemAddr{ k_inRexMovzxFromDisp32 });

		// A single-byte DEC instruction should be decoded instead of a REX prefix
		{
			const auto lengthDetails = decoder.GetNextLength();
			ASSERT(lengthDetails);
			EXPECT(!lengthDetails->prefixSeg);
			EXPECT(!lengthDetails->prefix66);
			EXPECT(!lengthDetails->prefix67);
			EXPECT(!lengthDetails->prefixRex);
			EXPECT(!lengthDetails->modRegRm);
			EXPECT(!lengthDetails->sib);
			EXPECT(!lengthDetails->dispNeedsFixup);
			EXPECT(lengthDetails->lengthOp == 1);
			EXPECT(lengthDetails->lengthDisp == 0);
			EXPECT(lengthDetails->lengthImm == 0);
			EXPECT(lengthDetails->GetLength() == 1);
		}

		// The second instruction should be identical to 32-bit version of "MovzxFromDisp32"
		{
			const auto lengthDetails = decoder.GetNextLength();
			ASSERT(lengthDetails);
			EXPECT(!lengthDetails->prefixSeg);
			EXPECT(!lengthDetails->prefix66);
			EXPECT(!lengthDetails->prefix67);
			EXPECT(!lengthDetails->prefixRex);
			EXPECT(lengthDetails->modRegRm);
			EXPECT(!lengthDetails->sib);
			EXPECT(!lengthDetails->dispNeedsFixup);
			EXPECT(lengthDetails->lengthOp == 2);
			EXPECT(lengthDetails->lengthDisp == 4);
			EXPECT(lengthDetails->lengthImm == 0);
			EXPECT(lengthDetails->GetLength() == 7);
		}
	}
	DEFINE_TEST_END


	// ---------------------------------------------------------------------------
	// Different semantics from AMD-64
	// ---------------------------------------------------------------------------

	DEFINE_TEST_START(MovImm32ToDisp32)
	{
		// mov  dword ptr ds:[44332211h], 88776655h
		const static uint8_t k_inMovImm32ToDisp32[] { 0xC7, 0x05, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };

		gan::InstructionDecoder decoder(gan::Arch::IA32, gan::ConstMemAddr{ k_inMovImm32ToDisp32 });
		const auto lengthDetails = decoder.GetNextLength();
		ASSERT(lengthDetails);
		EXPECT(!lengthDetails->prefixSeg);
		EXPECT(!lengthDetails->prefix66);
		EXPECT(!lengthDetails->prefix67);
		EXPECT(!lengthDetails->prefixRex);
		EXPECT(lengthDetails->modRegRm);
		EXPECT(!lengthDetails->sib);
		EXPECT(!lengthDetails->dispNeedsFixup);
		EXPECT(lengthDetails->lengthOp == 1);
		EXPECT(lengthDetails->lengthDisp == 4);
		EXPECT(lengthDetails->lengthImm == 4);
		EXPECT(lengthDetails->GetLength() == 10);
	}
	DEFINE_TEST_END

	DEFINE_TEST_START(MovFromMoff32)
	{
		// mov  eax, dword ptr ds:[44332211h]
		const static uint8_t k_inMovFromMoff32[]{ 0xA1, 0x11, 0x22, 0x33, 0x44 };

		gan::InstructionDecoder decoder(gan::Arch::IA32, gan::ConstMemAddr{ k_inMovFromMoff32 });
		const auto lengthDetails = decoder.GetNextLength();
		ASSERT(lengthDetails);
		EXPECT(!lengthDetails->prefixSeg);
		EXPECT(!lengthDetails->prefix66);
		EXPECT(!lengthDetails->prefix67);
		EXPECT(!lengthDetails->prefixRex);
		EXPECT(!lengthDetails->modRegRm);
		EXPECT(!lengthDetails->sib);
		EXPECT(!lengthDetails->dispNeedsFixup);
		EXPECT(lengthDetails->lengthOp == 1);
		EXPECT(lengthDetails->lengthDisp == 0);
		EXPECT(lengthDetails->lengthImm == 4);
		EXPECT(lengthDetails->GetLength() == 5);
	}
	DEFINE_TEST_END

	// Two-byte opcode
	DEFINE_TEST_START(MovzxFromDisp32)
	{
		// movzx  eax, byte ptr ds:[44332211h]
		const static uint8_t k_inMovzxFromDisp32[] { 0x0F, 0xB6, 0x05, 0x11, 0x22, 0x33, 0x44 };

		gan::InstructionDecoder decoder(gan::Arch::IA32, gan::ConstMemAddr{ k_inMovzxFromDisp32 });
		const auto lengthDetails = decoder.GetNextLength();
		ASSERT(lengthDetails);
		EXPECT(!lengthDetails->prefixSeg);
		EXPECT(!lengthDetails->prefix66);
		EXPECT(!lengthDetails->prefix67);
		EXPECT(!lengthDetails->prefixRex);
		EXPECT(lengthDetails->modRegRm);
		EXPECT(!lengthDetails->sib);
		EXPECT(!lengthDetails->dispNeedsFixup);
		EXPECT(lengthDetails->lengthOp == 2);
		EXPECT(lengthDetails->lengthDisp == 4);
		EXPECT(lengthDetails->lengthImm == 0);
		EXPECT(lengthDetails->GetLength() == 7);
	}
	DEFINE_TEST_END

	DEFINE_TEST_START(PushMem_WithoutSIB)
	{
		// push  dword ptr ds:[44332211h]
		const static uint8_t k_inPushMem[] { 0xFF, 0x35, 0x11, 0x22, 0x33, 0x44 };

		gan::InstructionDecoder decoder(gan::Arch::IA32, gan::ConstMemAddr{ k_inPushMem });
		auto lengthDetails = decoder.GetNextLength();
		ASSERT(lengthDetails);
		EXPECT(!lengthDetails->prefixSeg);
		EXPECT(!lengthDetails->prefix66);
		EXPECT(!lengthDetails->prefix67);
		EXPECT(!lengthDetails->prefixRex);
		EXPECT(lengthDetails->modRegRm);
		EXPECT(!lengthDetails->sib);
		EXPECT(!lengthDetails->dispNeedsFixup);
		EXPECT(lengthDetails->lengthOp == 1);
		EXPECT(lengthDetails->lengthDisp == 4);
		EXPECT(lengthDetails->lengthImm == 0);
		EXPECT(lengthDetails->GetLength() == 6);
	}
	DEFINE_TEST_END

	DEFINE_TEST_START(PushMem_WithSIB)
	{
		// push  dword ptr ds:[44332211h]
		const static uint8_t k_inPushMem[] { 0xFF, 0x34, 0x25, 0x11, 0x22, 0x33, 0x44 };

		gan::InstructionDecoder decoder(gan::Arch::IA32, gan::ConstMemAddr{ k_inPushMem });
		auto lengthDetails = decoder.GetNextLength();
		ASSERT(lengthDetails);
		EXPECT(!lengthDetails->prefixSeg);
		EXPECT(!lengthDetails->prefix66);
		EXPECT(!lengthDetails->prefix67);
		EXPECT(!lengthDetails->prefixRex);
		EXPECT(lengthDetails->modRegRm);
		EXPECT(lengthDetails->sib);
		EXPECT(!lengthDetails->dispNeedsFixup);
		EXPECT(lengthDetails->lengthOp == 1);
		EXPECT(lengthDetails->lengthDisp == 4);
		EXPECT(lengthDetails->lengthImm == 0);
		EXPECT(lengthDetails->GetLength() == 7);
	}
	DEFINE_TEST_END

	// ---------------------------------------------------------------------------
	// Identical semantics to AMD-64
	// ---------------------------------------------------------------------------

	DEFINE_TEST_START(MovImm32ToMem32)
	{
		// mov  dword ptr ds:[44332211h], 88776655h
		const static uint8_t k_inMovImm32ToMem32[] { 0xC7, 0x04, 0x25, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };

		gan::InstructionDecoder decoder(gan::Arch::IA32, gan::ConstMemAddr{ k_inMovImm32ToMem32 });
		const auto lengthDetails = decoder.GetNextLength();
		ASSERT(lengthDetails);
		EXPECT(!lengthDetails->prefixSeg);
		EXPECT(!lengthDetails->prefix66);
		EXPECT(!lengthDetails->prefix67);
		EXPECT(!lengthDetails->prefixRex);
		EXPECT(lengthDetails->modRegRm);
		EXPECT(lengthDetails->sib);
		EXPECT(!lengthDetails->dispNeedsFixup);
		EXPECT(lengthDetails->lengthOp == 1);
		EXPECT(lengthDetails->lengthDisp == 4);
		EXPECT(lengthDetails->lengthImm == 4);
		EXPECT(lengthDetails->GetLength() == 11);
	}
	DEFINE_TEST_END

	DEFINE_TEST_START(MovImm32ToDisp8)
	{
		// mov  dword ptr [esp + 4], 12345678h
		const static uint8_t k_inMovImm32ToDisp8[] { 0xC7, 0x44, 0x24, 0x04, 0x78, 0x56, 0x34, 0x12 };

		gan::InstructionDecoder decoder(gan::Arch::IA32, gan::ConstMemAddr{ k_inMovImm32ToDisp8 });
		const auto lengthDetails = decoder.GetNextLength();
		ASSERT(lengthDetails);
		EXPECT(!lengthDetails->prefixSeg);
		EXPECT(!lengthDetails->prefix66);
		EXPECT(!lengthDetails->prefix67);
		EXPECT(!lengthDetails->prefixRex);
		EXPECT(lengthDetails->modRegRm);
		EXPECT(lengthDetails->sib);
		EXPECT(!lengthDetails->dispNeedsFixup);
		EXPECT(lengthDetails->lengthOp == 1);
		EXPECT(lengthDetails->lengthDisp == 1);
		EXPECT(lengthDetails->lengthImm == 4);
		EXPECT(lengthDetails->GetLength() == 8);
	}
	DEFINE_TEST_END

	DEFINE_TEST_START(JmpNearAbsIndir)
	{
		// jmp  dword ptr [ebp + 12h]
		const static uint8_t k_inJmpNearAbsIndir[] { 0xFF, 0x64, 0x25, 0x12 };

		gan::InstructionDecoder decoder(gan::Arch::IA32, gan::ConstMemAddr{ k_inJmpNearAbsIndir });
		const auto lengthDetails = decoder.GetNextLength();
		ASSERT(lengthDetails);
		EXPECT(!lengthDetails->prefixSeg);
		EXPECT(!lengthDetails->prefix66);
		EXPECT(!lengthDetails->prefix67);
		EXPECT(!lengthDetails->prefixRex);
		EXPECT(lengthDetails->modRegRm);
		EXPECT(lengthDetails->sib);
		EXPECT(!lengthDetails->dispNeedsFixup);
		EXPECT(lengthDetails->lengthOp == 1);
		EXPECT(lengthDetails->lengthDisp == 1);
		EXPECT(lengthDetails->lengthImm == 0);
		EXPECT(lengthDetails->GetLength() == 4);
	}
	DEFINE_TEST_END

	// Opcode that has flag TreatImmAsDisp
	DEFINE_TEST_START(JmpNearRel)
	{
		// jmp  [eip + 44332211h]
		const static uint8_t k_inJmpNearRel[] { 0xE9, 0x11, 0x22, 0x33, 0x44 };

		gan::InstructionDecoder decoder(gan::Arch::IA32, gan::ConstMemAddr{ k_inJmpNearRel });
		const auto lengthDetails = decoder.GetNextLength();
		ASSERT(lengthDetails);
		EXPECT(!lengthDetails->prefixSeg);
		EXPECT(!lengthDetails->prefix66);
		EXPECT(!lengthDetails->prefix67);
		EXPECT(!lengthDetails->prefixRex);
		EXPECT(!lengthDetails->modRegRm);
		EXPECT(!lengthDetails->sib);
		EXPECT(lengthDetails->dispNeedsFixup);
		EXPECT(lengthDetails->lengthOp == 1);
		EXPECT(lengthDetails->lengthDisp == 4);
		EXPECT(lengthDetails->lengthImm == 0);
		EXPECT(lengthDetails->GetLength() == 5);
	}
	DEFINE_TEST_END

	// Single-byte instruction
	DEFINE_TEST_START(PushEax)
	{
		// push  eax
		const static uint8_t k_inPushR_Eax[] { 0x50 };

		gan::InstructionDecoder decoder(gan::Arch::IA32, gan::ConstMemAddr{ k_inPushR_Eax });
		const auto lengthDetails = decoder.GetNextLength();
		ASSERT(lengthDetails);
		EXPECT(!lengthDetails->prefixSeg);
		EXPECT(!lengthDetails->prefix66);
		EXPECT(!lengthDetails->prefix67);
		EXPECT(!lengthDetails->prefixRex);
		EXPECT(!lengthDetails->modRegRm);
		EXPECT(!lengthDetails->sib);
		EXPECT(!lengthDetails->dispNeedsFixup);
		EXPECT(lengthDetails->lengthOp == 1);
		EXPECT(lengthDetails->lengthDisp == 0);
		EXPECT(lengthDetails->lengthImm == 0);
		EXPECT(lengthDetails->GetLength() == 1);
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END
