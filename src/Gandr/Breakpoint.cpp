/*
 *  Gandr - another minimalism library for hacking x86-based Windows
 *  Copyright (C) 2020-2025 Mifan Bang <https://debug.tw>.
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

#include <Breakpoint.h>

#include <windows.h>


namespace
{


enum class Dr7UpdateOp : uint8_t
{
	Enable,
	Disable
};


constexpr size_t GetMaskFromSlot(gan::HWBreakpointSlot slot) noexcept
{
	// Using local mode (L0-L3), one-byte length (LEN=0), breaking on execution only
	return size_t{ 1 } << (static_cast<uint8_t>(slot) << 1);
}


bool UpdateDebugRegisters(gan::WinHandle hThread, gan::ConstMemAddr address, gan::HWBreakpointSlot slot, Dr7UpdateOp opDr7) noexcept
{
	CONTEXT ctx{
		.ContextFlags = CONTEXT_DEBUG_REGISTERS
	};
	if (::GetThreadContext(hThread, &ctx) == 0)
		return false;

	// Write to the corresponding register field in "ctx".
	const auto ptrRegInCtx =
		gan::MemAddr{ &ctx.Dr0 }
		.Offset(sizeof(gan::MemAddr) * static_cast<uint8_t>(slot));
	ptrRegInCtx.Ref<gan::ConstMemAddr>() = address;

	// Setup control register
	if (opDr7 == Dr7UpdateOp::Enable)
		ctx.Dr7 |= GetMaskFromSlot(slot);
	else
		ctx.Dr7 &= ~GetMaskFromSlot(slot);

	ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
	return ::SetThreadContext(hThread, &ctx) != 0;
}


}  // unnamed namespace


namespace gan
{


bool HWBreakpoint::Enable(WinHandle thread, ConstMemAddr addr, HWBreakpointSlot slot) noexcept
{
	return UpdateDebugRegisters(thread, addr, slot, Dr7UpdateOp::Enable);
}


bool HWBreakpoint::Disable(WinHandle thread, HWBreakpointSlot slot) noexcept
{
	return UpdateDebugRegisters(thread, ConstMemAddr{ }, slot, Dr7UpdateOp::Disable);
}


}  // namespace gan
