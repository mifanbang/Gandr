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


namespace gan
{


// ---------------------------------------------------------------------------
// Class Hook: Just a hook.
//
// The following Win32 API functions are used by Hook and shouldn't be hooked:
//   - GetSystemInfo()
// 	 - VirtualAlloc()
//   - VirtualProtect()
//   - VirtualQuery()
// ---------------------------------------------------------------------------

class Hook
{
public:
	enum class OpResult : uint8_t
	{
		Hooked,
		Unhooked,

		// Warnings
		NotHooked,				// Caused by uninstalling an function not previously hooked.
		AddressInUse,			// A previous hook installed by Gandr is still active.

		// Errors
		PrologNotSupported,		// Failed to decode the instructions in target prolog.
		TrampolineAllocFailed,	// Failed to allocate memory for a new trampoline.
		PrologMismatched,		// Failed to uninstall due to our hook getting hooked by something else.
		AccessDenied,			// Failed to write to memory.
	};

	template <class F>
		requires IsAnyFuncPtr<F>
	Hook(F origFunc, F hookFunc) noexcept
		: m_funcOrig(FromAnyFn(origFunc))
		, m_funcHook(FromAnyFn(hookFunc))
		, m_hooked(false)
	{
		AssertCtorArgs(m_funcOrig, m_funcHook);
	}

	OpResult Install();
	OpResult Uninstall();

	template <class F>
		requires IsAnyFuncPtr<F>
	static auto GetTrampoline(F origFunc)
	{
		return ToAnyFn<F>(
			GetTrampolineAddr(gan::ConstMemAddr{ FromAnyFn(origFunc) })
			.ConstCast()
			.Ptr<>()
		);
	}

	// Unsafe because of the casting from raw pointer to function pointer.
	template <class F>
		requires IsAnyFuncPtr<F>
	static auto GetTrampolineUnsafe(gan::ConstMemAddr origFunc)
	{
		return ToAnyFn<F>(GetTrampolineAddr(origFunc).ConstCast().Ptr<>());
	}

private:
	// Helper functions as a layer of abstraction not to expose implementation in header.
	static void AssertCtorArgs(MemAddr origFunc, MemAddr hookFunc) noexcept;
	static ConstMemAddr GetTrampolineAddr(ConstMemAddr origFunc);  // Usage of this function is highly discouraged.

	MemAddr m_funcOrig;  // address to where the inline hook is installed.
	MemAddr m_funcHook;  // address to the user-defined hook function.
	bool m_hooked;
};


}  // namespace gan
