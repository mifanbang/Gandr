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

#include <InstructionDecoder.h>

#include <Types.h>

#include <cassert>



namespace
{


// excluding the higher half 0x40
struct PrefixREX
{
	bool b : 1;
	bool x : 1;
	bool r : 1;
	bool w : 1;
};


struct ModRegRM
{
	uint8_t rm : 3;
	uint8_t reg : 3;
	uint8_t mod : 2;
};


struct SIB
{
	uint8_t base : 3;
	uint8_t index : 3;
	uint8_t scale : 2;
};


struct Opcode
{
	uint8_t length;
	union
	{
		uint8_t bytes[2];  // The longest opcode is 3 bytes but Gandr currently supports up to 2 bytes
		short op16i;  // An integral for fast matching between two opcodes
	};

	constexpr Opcode(uint8_t opcode)
		: length(1)
		, bytes{ opcode, 0 }
	{ }

	constexpr Opcode(uint8_t opcode_1, uint8_t opcode_2)
		: length(2)
		, bytes{ opcode_1, opcode_2 }
	{ }

	constexpr bool operator == (Opcode other) const
	{
		return length == other.length && op16i == other.op16i;
	}
};


// Looks ahead and decodes the length of opcode
struct OpcodeLookAhead
{
	Opcode opcode;
	ModRegRM modRegRM;
	SIB sib;

	OpcodeLookAhead(gan::ConstMemAddr addr)
		: opcode(addr.ConstRef<uint8_t>())
		, modRegRM()
		, sib()
	{
		// The longest opcode is 3 bytes but Gandr currently supports up to 2 bytes
		if (opcode.bytes[0] == 0x0F)
			opcode = Opcode(0x0F, addr.Offset(1).ConstRef<uint8_t>());

		const auto ptrEndOfOpcode = addr.Offset(opcode.length);
		modRegRM = ptrEndOfOpcode.ConstRef<ModRegRM>();
		sib = ptrEndOfOpcode.Offset(1).ConstRef<SIB>();
	}
};


enum class RegField : uint8_t
{
	R0, R1, R2, R3, R4, R5, R6, R7, Unused
};


enum class Operand : uint8_t
{
	Imm8		= 1,
	Imm16		= 1 << 1,
	Imm32		= 1 << 2,
	Reg			= 1 << 3,
	R_M			= 1 << 4,
	Moffs		= 1 << 5,  // Memory offsets; only used in MOV
	InOpcode	= 1 << 6,  // Operand encoded in the lowest 3 bits of opcode
};


enum class MiscFlags : uint8_t
{
	OpInModRegRM	= 1,  // Reg field in ModRegR/M is used as a part of opcode
	Imm64Support	= 1 << 1,  // When REX.W is present, imm32 expands to imm64
	TreatImmAsDisp	= 1 << 2,  // A special case for instructions that do *NOT* use ModRegR/M for disp; applies to all arch's
	IA32Only		= 1 << 3,
};


template <typename T>
constexpr uint8_t MakeFlags(T flag)
{
	return static_cast<uint8_t>(flag);
}
template <typename F, typename ...T>
constexpr uint8_t MakeFlags(F f, T... flags)
{
	return static_cast<uint8_t>(f) | MakeFlags(flags...);
}

template <typename T>
constexpr bool HasAnyFlagIn(uint8_t flags, T desiredFlags)
{
	return flags & static_cast<uint8_t>(desiredFlags);
}


struct OpcodeDefinition
{
	Opcode opcode;
	RegField reg;  // reg field in ModRegR/M; ignored when operands doesn't have Operand::ModRegRM
	uint8_t operands;
	uint8_t flags;

	constexpr OpcodeDefinition(Opcode opcode)
		: OpcodeDefinition(opcode, RegField::Unused, 0, 0)
	{ }

	constexpr OpcodeDefinition(Opcode opcode, uint8_t operands)
		: OpcodeDefinition(opcode, RegField::Unused, operands, 0)
	{ }

	constexpr OpcodeDefinition(Opcode opcode, uint8_t operands, uint8_t flags)
		: OpcodeDefinition(opcode, RegField::Unused, operands, flags)
	{ }

	constexpr OpcodeDefinition(Opcode opcode, RegField reg, uint8_t operands)
		: OpcodeDefinition(opcode, reg, operands, MakeFlags(MiscFlags::OpInModRegRM))
	{ }

	constexpr OpcodeDefinition(Opcode opcode, RegField reg, uint8_t operands, uint8_t flags)
		: opcode(opcode)
		, reg(reg)
		, operands(operands)
		, flags(reg == RegField::Unused ? flags : MakeFlags(flags, MiscFlags::OpInModRegRM))
	{ }
};



constexpr OpcodeDefinition k_opDefTable[] {
	// ADD
	{ 0x00,					MakeFlags(Operand::R_M, Operand::Reg) },
	{ 0x01,					MakeFlags(Operand::R_M, Operand::Reg) },
	{ 0x02,					MakeFlags(Operand::Reg, Operand::R_M) },
	{ 0x03,					MakeFlags(Operand::Reg, Operand::R_M) },
	{ 0x04,					MakeFlags(Operand::Imm8) },
	{ 0x05,					MakeFlags(Operand::Imm32) },
	{ 0x80,	RegField::R0,	MakeFlags(Operand::R_M, Operand::Imm8) },
	{ 0x81,	RegField::R0,	MakeFlags(Operand::R_M, Operand::Imm32) },
	{ 0x83,	RegField::R0,	MakeFlags(Operand::R_M, Operand::Imm8) },

	// AND
	{ 0x20,					MakeFlags(Operand::R_M, Operand::Reg) },
	{ 0x21,					MakeFlags(Operand::R_M, Operand::Reg) },
	{ 0x22,					MakeFlags(Operand::Reg, Operand::R_M) },
	{ 0x23,					MakeFlags(Operand::Reg, Operand::R_M) },
	{ 0x24,					MakeFlags(Operand::Imm8) },
	{ 0x25,					MakeFlags(Operand::Imm32) },
	{ 0x80, RegField::R4,	MakeFlags(Operand::R_M, Operand::Imm8) },
	{ 0x81, RegField::R4,	MakeFlags(Operand::R_M, Operand::Imm32) },
	{ 0x83, RegField::R4,	MakeFlags(Operand::R_M, Operand::Imm8) },

	// CMP
	{ 0x38,					MakeFlags(Operand::R_M, Operand::Reg) },
	{ 0x39,					MakeFlags(Operand::R_M, Operand::Reg) },
	{ 0x3A,					MakeFlags(Operand::Reg, Operand::R_M) },
	{ 0x3B,					MakeFlags(Operand::Reg, Operand::R_M) },
	{ 0x3C,					MakeFlags(Operand::Imm8) },
	{ 0x3D,					MakeFlags(Operand::Imm32) },
	{ 0x80,	RegField::R7,	MakeFlags(Operand::R_M, Operand::Imm8) },
	{ 0x81,	RegField::R7,	MakeFlags(Operand::R_M, Operand::Imm32) },
	{ 0x83,	RegField::R7,	MakeFlags(Operand::R_M, Operand::Imm8) },

	// DEC
	{ 0x48,					MakeFlags(Operand::InOpcode), MakeFlags(MiscFlags::IA32Only) },
	{ 0xFE,	RegField::R1,	MakeFlags(Operand::R_M) },
	{ 0xFF,	RegField::R1,	MakeFlags(Operand::R_M) },

	// INT3
	{ 0xCC },

	// JMP
	{ 0xE9,					MakeFlags(Operand::Imm32),	MakeFlags(MiscFlags::TreatImmAsDisp) },
	{ 0xFF,	RegField::R4,	MakeFlags(Operand::R_M) },
	// No support for opcodes "0xEB", "0xEA" and "0xFF /5".

	// LEA
	{ 0x8D,					MakeFlags(Operand::Reg, Operand::R_M) },

	// MOV
	{ 0x88,					MakeFlags(Operand::R_M, Operand::Reg) },
	{ 0x89,					MakeFlags(Operand::R_M, Operand::Reg) },
	{ 0x8A,					MakeFlags(Operand::Reg, Operand::R_M) },
	{ 0x8B,					MakeFlags(Operand::Reg, Operand::R_M) },
	{ 0x8C,					MakeFlags(Operand::R_M, Operand::Reg) },
	{ 0x8E,					MakeFlags(Operand::Reg, Operand::R_M) },
	{ 0xA0,					MakeFlags(Operand::Moffs) },
	{ 0xA1,					MakeFlags(Operand::Moffs) },
	{ 0xA2,					MakeFlags(Operand::Moffs) },
	{ 0xA3,					MakeFlags(Operand::Moffs) },
	{ 0xB0,					MakeFlags(Operand::InOpcode, Operand::Imm8)},
	{ 0xB8,					MakeFlags(Operand::InOpcode, Operand::Imm32),	MakeFlags(MiscFlags::Imm64Support) },
	{ 0xC6,	RegField::R0,	MakeFlags(Operand::R_M, Operand::Imm8) },
	{ 0xC7,	RegField::R0,	MakeFlags(Operand::R_M, Operand::Imm32) },

	// MOVZX
	{ { 0x0F, 0xB6 },		MakeFlags(Operand::Reg, Operand::R_M) },
	{ { 0x0F, 0xB7 },		MakeFlags(Operand::Reg, Operand::R_M) },

	// NOP
	{ 0xC9 },

	// OR
	{ 0x08,					MakeFlags(Operand::R_M, Operand::Reg) },
	{ 0x09,					MakeFlags(Operand::R_M, Operand::Reg) },
	{ 0x0A,					MakeFlags(Operand::Reg, Operand::R_M) },
	{ 0x0B,					MakeFlags(Operand::Reg, Operand::R_M) },
	{ 0x0C,					MakeFlags(Operand::Imm8) },
	{ 0x0D,					MakeFlags(Operand::Imm32) },
	{ 0x80, RegField::R1,	MakeFlags(Operand::R_M, Operand::Imm8) },
	{ 0x81, RegField::R1,	MakeFlags(Operand::R_M, Operand::Imm32) },
	{ 0x83, RegField::R1,	MakeFlags(Operand::R_M, Operand::Imm8) },

	// PUSH
	{ 0x06,					0 },
	{ 0x0E,					0 },
	{ 0x16,					0 },
	{ 0x1E,					0 },
	{ 0x50,					MakeFlags(Operand::InOpcode) },
	{ 0x68,					MakeFlags(Operand::Imm32) },
	{ 0x6A,					MakeFlags(Operand::Imm8) },
	{ 0xFF,	RegField::R6,	MakeFlags(Operand::R_M) },

	// RET
	{ 0xC3 },

	// SUB
	{ 0x28,					MakeFlags(Operand::R_M, Operand::Reg) },
	{ 0x29,					MakeFlags(Operand::R_M, Operand::Reg) },
	{ 0x2A,					MakeFlags(Operand::Reg, Operand::R_M) },
	{ 0x2B,					MakeFlags(Operand::Reg, Operand::R_M) },
	{ 0x2C,					MakeFlags(Operand::Imm8) },
	{ 0x2D,					MakeFlags(Operand::Imm32) },
	{ 0x80,	RegField::R5,	MakeFlags(Operand::R_M, Operand::Imm8) },
	{ 0x81,	RegField::R5,	MakeFlags(Operand::R_M, Operand::Imm32) },
	{ 0x83,	RegField::R5,	MakeFlags(Operand::R_M, Operand::Imm8) },

	// XOR
	{ 0x30,					MakeFlags(Operand::R_M, Operand::Reg) },
	{ 0x31,					MakeFlags(Operand::R_M, Operand::Reg) },
	{ 0x32,					MakeFlags(Operand::Reg, Operand::R_M) },
	{ 0x33,					MakeFlags(Operand::Reg, Operand::R_M) },
	{ 0x34,					MakeFlags(Operand::Imm8) },
	{ 0x35,					MakeFlags(Operand::Imm32) },
	{ 0x80,	RegField::R6,	MakeFlags(Operand::R_M, Operand::Imm8) },
	{ 0x81,	RegField::R6,	MakeFlags(Operand::R_M, Operand::Imm32) },
	{ 0x83,	RegField::R6,	MakeFlags(Operand::R_M, Operand::Imm8) },
};



std::optional<gan::InstructionLengthDetails> GenerateLengthInfo(gan::Arch arch, gan::ConstMemAddr addr)
{
	gan::InstructionLengthDetails result;

	// extract all prefixes
	for (; ; ++addr)
	{
		const auto byte = addr.ConstRef<uint8_t>();
		if ((byte == 0x2E || byte == 0x36 || byte == 0x26 || byte == 0x64 || byte == 0x65) && !result.prefixSeg)
		{
			result.prefixSeg = true;
			continue;
		}
		else if (byte == 0x66 && !result.prefix66)
		{
			result.prefix66 = true;
			continue;
		}
		else if (byte == 0x67 && !result.prefix67)
		{
			result.prefix67 = true;
			continue;
		}
		else if (arch == gan::Arch::Amd64 && (byte & 0xF0) == 0x40)
		{
			// REX must be the last prefix before opcode, so fallthrough to break.
			result.prefixRex = true;
			++addr;
		}
		break;
	}

	std::optional<OpcodeDefinition> matchedOp;
	const OpcodeLookAhead lookAhead(addr);  // prefetch stuff

	// Match opcode in k_opDefTable
	for (const OpcodeDefinition& opDef : k_opDefTable)
	{
		if (HasAnyFlagIn(opDef.flags, MiscFlags::IA32Only) && arch != gan::Arch::IA32)
			continue;

		if (HasAnyFlagIn(opDef.operands, Operand::InOpcode)
			&& lookAhead.opcode.length == opDef.opcode.length
			&& (lookAhead.opcode.op16i & 0xF8) == opDef.opcode.op16i)
		{
			matchedOp = opDef;
			break;
		}
		else if (lookAhead.opcode == opDef.opcode)
		{
			if (HasAnyFlagIn(opDef.flags, MiscFlags::OpInModRegRM)
				&& lookAhead.modRegRM.reg != static_cast<uint8_t>(opDef.reg))
			{
				continue;
			}

			matchedOp = opDef;
			break;
		}
	}

	if (!matchedOp)
		return std::nullopt;

	// Now we have recognized the opcode

	result.lengthOp = lookAhead.opcode.length;
	result.modRegRm = HasAnyFlagIn(matchedOp->operands, MakeFlags(Operand::Reg, Operand::R_M));
	result.sib = HasAnyFlagIn(matchedOp->operands, Operand::R_M)
		&& lookAhead.modRegRM.mod != 0b11
		&& lookAhead.modRegRM.rm == 0b100;

	// displacement
	if (HasAnyFlagIn(matchedOp->operands, Operand::R_M))
	{
		if (lookAhead.modRegRM.mod == 0b01)
			result.lengthDisp = 1;
		else if (lookAhead.modRegRM.mod == 0b10)
			result.lengthDisp = result.prefix66 ? 2 : 4;
		else if (lookAhead.modRegRM.mod == 0b00 && lookAhead.modRegRM.rm == 0b101)
		{
			result.dispNeedsFixup = (arch == gan::Arch::Amd64);
			result.lengthDisp = 4;
		}
		else if (result.sib && lookAhead.sib.base == 0b101)
			result.lengthDisp = lookAhead.modRegRM.mod == 0b01 ? 1 : 4;
	}

	// immediate
	if (HasAnyFlagIn(matchedOp->operands, Operand::Imm8))
		result.lengthImm = 1;
	else if (HasAnyFlagIn(matchedOp->operands, Operand::Imm16))
		result.lengthImm = 2;
	else if (HasAnyFlagIn(matchedOp->operands, Operand::Imm32))
	{
		if (HasAnyFlagIn(matchedOp->flags, MiscFlags::Imm64Support) && result.prefixRex)
			result.lengthImm = 8;
		else
			result.lengthImm = result.prefix66 ? 2 : 4;
	}

	// Moffs (memory offsets)
	// This is a very rare case but it's simple enough to support.
	else if (HasAnyFlagIn(matchedOp->operands, Operand::Moffs))
	{
		if constexpr (gan::Is64())
			result.lengthImm = result.prefix67 ? 4 : 8;
		else
			result.lengthImm = result.prefix67 ? 2 : 4;
	}

	// Handling of special flags
	if (HasAnyFlagIn(matchedOp->flags, MiscFlags::TreatImmAsDisp))
	{
		assert(result.lengthDisp == 0);  // Instruction must *NOT* already have disp
		result.dispNeedsFixup = true;  // Don't care architecture
		result.lengthDisp = result.lengthImm;
		result.lengthImm = 0;
	}

	return result;
}


}  // unnamed namespace



namespace gan
{


InstructionDecoder::InstructionDecoder(Arch arch, ConstMemAddr address)
	: m_instPtr(address)
	, m_arch(arch)
{
	assert(arch == Arch::IA32 || arch == Arch::Amd64);
}


std::optional<InstructionLengthDetails> InstructionDecoder::GetNextLength()
{
	if (!m_instPtr)
		return std::nullopt;

	if (const auto lengthInfo = GenerateLengthInfo(m_arch, m_instPtr))
	{
		const uint8_t length = lengthInfo->GetLength();
		m_instPtr = m_instPtr.Offset(length);
		return lengthInfo;
	}
	return std::nullopt;
}



}  // namespace gan
