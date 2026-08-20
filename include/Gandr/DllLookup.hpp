/*
 *  Gandr - another minimalism library for hacking x86-based Windows
 *  Copyright (C) 2020-2026 Mifan Bang <https://debug.tw>.
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

#include <Gandr/Types.hpp>

#include <string_view>


namespace gan
{

// ---------------------------------------------------------------------------
// Class DllLookup - Resolve a symbol with the specified dynamic lib.
//					 Loead the library when needed.
// ---------------------------------------------------------------------------

class DllLookup
{
public:
	template <class T>
	static auto Get(std::wstring_view lib, std::string_view name)
	{
		if constexpr (IsAnyFuncPtr<T>)
			return ToAnyFn<T>(LoadLibAndGetSymbol(lib, name));
		else
			return reinterpret_cast<T>(LoadLibAndGetSymbol(lib, name));
	}

private:
	static void* LoadLibAndGetSymbol(std::wstring_view lib, std::string_view name);
};

}  // namespace gan
