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

#include <string_view>


namespace gan
{


// ---------------------------------------------------------------------------
// Class DynamicCall - Dynamically load and resolve a function
// ---------------------------------------------------------------------------

class DynamicCall
{
public:
	template <class F>
		requires IsAnyFuncPtr<F>
	static F Get(std::wstring_view lib, std::string_view func)
	{
		return ToAnyFn<F>(LoadLibAndGetProc(lib, func));
	}

private:
	static void* LoadLibAndGetProc(std::wstring_view lib, std::string_view func);
};


}  // namespace gan
