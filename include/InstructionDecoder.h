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

#pragma once

#include <Types.h>

#include <cstdint>
#include <optional>



namespace gan
{


struct InstructionLengthDetails
{
	bool prefixSeg : 1;  // Segment override: any of 0x2E, 0x36, 0x3E, 0x26, 0x64, and 0x65
	bool prefix66 : 1;  // Operand-size override
	bool prefix67 : 1;  // Address-size override
	bool prefixRex : 1;
	bool modRegRm : 1;
	bool sib : 1;
	bool dispNeedsFixup : 1;  // This bit is set when disp is IP-based
	uint8_t lengthOp;
	uint8_t lengthDisp;
	uint8_t lengthImm;

	constexpr uint8_t GetLength() const
	{
		return prefixSeg + prefix66 + prefix67 + prefixRex
			+ modRegRm + sib
			+ lengthOp
			+ lengthDisp + lengthImm;
	}

	constexpr InstructionLengthDetails()
		: prefixSeg(false)
		, prefix66(false)
		, prefix67(false)
		, prefixRex(false)
		, modRegRm(false)
		, sib(false)
		, dispNeedsFixup(false)
		, lengthOp(0)
		, lengthDisp(0)
		, lengthImm(0)
	{ }
};


class InstructionDecoder
{
public:
	InstructionDecoder(Arch arch, ConstMemAddr address);

	// Using build's target platform as arch
	explicit InstructionDecoder(ConstMemAddr address)
		: InstructionDecoder(k_64bit ? Arch::Amd64 : Arch::IA32, address)
	{ }

	std::optional<InstructionLengthDetails> GetNextLength();

private:
	ConstMemAddr m_instPtr;
	Arch m_arch;
};


}  // namespace gan
