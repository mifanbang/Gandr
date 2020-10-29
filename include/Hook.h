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

#include <Types.h>

#include <cassert>
#include <cstdint>


namespace gan
{


// ---------------------------------------------------------------------------
// Class Hook: Just a hook.
//
// The following Win32 API functions are used by Hook and shouldn't be hooked:
//   - GetSystemInfo()
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

		// The followings are errors
		AddressInUse,			// A previous hook installed by Gandr is still active.
		PrologNotSupported,		// Failed to decode the instructions in target prolog.
		TrampolineAllocFailed,	// Failed to allocate memory for a new trampoline.
		PrologMismatched,		// Failed to uninstall due to our hook getting hooked by something else.
		AccessDenied,			// Failed to write to memory.
	};


	template <typename F>
	constexpr Hook(const F* origFunc, const F* hookFunc)
		: m_funcOrig(origFunc)
		, m_funcHook(hookFunc)
		, m_hooked(false)
	{
		assert(m_funcOrig);
		assert(m_funcHook);
		assert(m_funcOrig != m_funcHook);
	}

	OpResult Install();
	OpResult Uninstall();


	template <typename F>
	static auto GetTrampoline(const F& origFunc)
	{
		static_assert(std::is_function<F>::value);
		auto trampoline = GetTrampolineAddr(origFunc);
		assert(trampoline);
		return trampoline.As<F>();
	}


private:
	// for hiding implementation as CallTrampoline must be defined in header
	static MemAddr GetTrampolineAddr(MemAddr origFunc);

	MemAddr m_funcOrig;  // address to where the inline hook is installed
	MemAddr m_funcHook;  // address to the user-defined hook function
	bool m_hooked;
};


}  // namespace gan
