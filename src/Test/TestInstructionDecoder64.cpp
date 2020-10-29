/*
 *  Gandr - another minimalism library for hacking x86-based Windows
 *  Copyright (C) 2020 Mifan Bang <https://debug.tw>.
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

#pragma once

#include "Test.h"

#include <InstructionDecoder.h>



DEFINE_TESTSUITE_START(InstructionDecoder_Amd64)

	// ---------------------------------------------------------------------------
	// AMD64-only
	// ---------------------------------------------------------------------------

	DEFINE_TEST_START(MovImm64ToReg64)
	{
		// REX.WB mov  r15, 0BBAA785600003412h
		const static uint8_t k_inMovImm64ToReg64[] { 0x49, 0xBF, 0x12, 0x34, 0, 0, 0x56, 0x78, 0xAA, 0xBB };

		gan::InstructionDecoder decoder(gan::Arch::Amd64, k_inMovImm64ToReg64);
		const auto lengthDetails = decoder.GetNextLength();
		ASSERT(lengthDetails);
		EXPECT(lengthDetails->prefixRex);
		EXPECT(!lengthDetails->dispNeedsFixup);
		EXPECT(lengthDetails->lengthOp == 1);
		EXPECT(lengthDetails->lengthDisp == 0);
		EXPECT(lengthDetails->lengthImm == 8);
		EXPECT(lengthDetails->GetLength() == 10);
	}
	DEFINE_TEST_END

	// ---------------------------------------------------------------------------
	// Different semantics from IA-32
	// ---------------------------------------------------------------------------

	DEFINE_TEST_START(MovImm32ToDisp32)
	{
		// mov  dword ptr [rip + 44332211h], 88776655h
		const static uint8_t k_inMovImm32ToDisp32[] { 0xC7, 0x05, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };

		gan::InstructionDecoder decoder(gan::Arch::Amd64, k_inMovImm32ToDisp32);
		const auto lengthDetails = decoder.GetNextLength();
		ASSERT(lengthDetails);
		EXPECT(lengthDetails->modRegRm);
		EXPECT(lengthDetails->dispNeedsFixup);
		EXPECT(lengthDetails->lengthOp == 1);
		EXPECT(lengthDetails->lengthImm == 4);
		EXPECT(lengthDetails->lengthDisp == 4);
		EXPECT(lengthDetails->GetLength() == 10);
	}
	DEFINE_TEST_END

	// Two-byte opcode
	DEFINE_TEST_START(MovzxFromDisp32)
	{
		// REX.W movzx  rax, byte ptr [74172A1A21h]
		const static uint8_t k_inMovzxFromDisp32[] { 0x48, 0x0F, 0xB6, 0x05, 0x11, 0x22, 0x33, 0x44 };

		gan::InstructionDecoder decoder(gan::Arch::Amd64, k_inMovzxFromDisp32);
		const auto lengthDetails = decoder.GetNextLength();
		ASSERT(lengthDetails);
		EXPECT(lengthDetails->prefixRex);
		EXPECT(lengthDetails->modRegRm);
		EXPECT(lengthDetails->dispNeedsFixup);
		EXPECT(lengthDetails->lengthOp == 2);
		EXPECT(lengthDetails->lengthDisp == 4);
		EXPECT(lengthDetails->lengthImm == 0);
		EXPECT(lengthDetails->GetLength() == 8);
	}
	DEFINE_TEST_END

	DEFINE_TEST_START(PushMem_WithoutSIB)
	{
		// push  qword ptr [rip + 44332211h]
		const static uint8_t k_inPushMem[] { 0xFF, 0x35, 0x11, 0x22, 0x33, 0x44 };

		gan::InstructionDecoder decoder(gan::Arch::Amd64, k_inPushMem);
		auto lengthDetails = decoder.GetNextLength();
		ASSERT(lengthDetails);
		EXPECT(lengthDetails->modRegRm);
		EXPECT(!lengthDetails->sib);
		EXPECT(lengthDetails->dispNeedsFixup);
		EXPECT(lengthDetails->lengthOp == 1);
		EXPECT(lengthDetails->lengthDisp == 4);
		EXPECT(lengthDetails->lengthImm == 0);
		EXPECT(lengthDetails->GetLength() == 6);
	}
	DEFINE_TEST_END

	DEFINE_TEST_START(PushMem_WithSIB)
	{
		// push  qword ptr [44332211h]
		const static uint8_t k_inPushMem[] { 0xFF, 0x34, 0x25, 0x11, 0x22, 0x33, 0x44 };

		gan::InstructionDecoder decoder(gan::Arch::Amd64, k_inPushMem);
		auto lengthDetails = decoder.GetNextLength();
		ASSERT(lengthDetails);
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
	// Identical semantics to IA-32
	// ---------------------------------------------------------------------------

	DEFINE_TEST_START(MovImm32ToMem32)
	{
		// mov  dword ptr [44332211h], 88776655h
		const static uint8_t k_inMovImm32ToMem32[] { 0xC7, 0x04, 0x25, 0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88 };

		gan::InstructionDecoder decoder(gan::Arch::Amd64, k_inMovImm32ToMem32);
		const auto lengthDetails = decoder.GetNextLength();
		ASSERT(lengthDetails);
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

		gan::InstructionDecoder decoder(gan::Arch::Amd64, k_inMovImm32ToDisp8);
		const auto lengthDetails = decoder.GetNextLength();
		ASSERT(lengthDetails);
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
		// jmp  qword ptr [rbp + 12h]
		const static uint8_t k_inJmpNearAbsIndir[] { 0xFF, 0x64, 0x25, 0x12 };

		gan::InstructionDecoder decoder(gan::Arch::Amd64, k_inJmpNearAbsIndir);
		const auto lengthDetails = decoder.GetNextLength();
		ASSERT(lengthDetails);
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
		// jmp  [rip + 44332211h]
		const static uint8_t k_inJmpNearRel[] { 0xE9, 0x11, 0x22, 0x33, 0x44 };

		gan::InstructionDecoder decoder(gan::Arch::Amd64, k_inJmpNearRel);
		const auto lengthDetails = decoder.GetNextLength();
		ASSERT(lengthDetails);
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
	DEFINE_TEST_START(PushRax)
	{
		// push  rax
		const static uint8_t k_inPushR_Eax[] { 0x50 };

		gan::InstructionDecoder decoder(gan::Arch::Amd64, k_inPushR_Eax);
		const auto lengthDetails = decoder.GetNextLength();
		ASSERT(lengthDetails);
		EXPECT(!lengthDetails->dispNeedsFixup);
		EXPECT(lengthDetails->lengthOp == 1);
		EXPECT(lengthDetails->lengthDisp == 0);
		EXPECT(lengthDetails->lengthImm == 0);
		EXPECT(lengthDetails->GetLength() == 1);
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END

