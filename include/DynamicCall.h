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

#include <utility>


namespace gan
{



class DynamicCallBase
{
protected:
	// Loads library if not loaded before.
	static void* ObtainFunction(const wchar_t* libName, const char* funcName);
};



// ---------------------------------------------------------------------------
// Class DynamicCall - Dynamically loading and calling a function
// ---------------------------------------------------------------------------

template <typename T>
class DynamicCall : private DynamicCallBase
{
public:
	DynamicCall(const wchar_t* libName, const char* funcName)
		: m_pFunc(nullptr)
	{
		m_pFunc = reinterpret_cast<T*>(ObtainFunction(libName, funcName));
	}

	inline bool IsValid() const
	{
		return m_pFunc != nullptr;
	}

	inline T* GetAddress() const
	{
		return m_pFunc;
	}

	template <typename... Arg>
	auto operator()(Arg&&... arg) const
	{
		return m_pFunc(std::forward<Arg>(arg)...);
	}


private:
	T* m_pFunc;
};



}  // namespace gan
