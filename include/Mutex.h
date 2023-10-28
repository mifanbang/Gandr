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

#include <windows.h>

#include <type_traits>


namespace gan
{



template <typename T>
class ThreadSafeResource
{
public:
	template <typename... Arg>
	ThreadSafeResource(Arg&&... arg)
		: m_resInst(std::forward<Arg>(arg)...)
	{
		::InitializeCriticalSection(&m_lock);
	}

	~ThreadSafeResource()
	{
		::DeleteCriticalSection(&m_lock);
	}

	ThreadSafeResource(const ThreadSafeResource<T>&) = delete;
	ThreadSafeResource(ThreadSafeResource<T>&&) = delete;

	template <typename F>
	auto ApplyOperation(const F& func)
	{
		constexpr bool HasReturnValue = std::is_same_v<void, decltype(func(m_resInst))>;

		::EnterCriticalSection(&m_lock);
		if constexpr (HasReturnValue)
		{
			func(m_resInst);
			::LeaveCriticalSection(&m_lock);
		}
		else
		{
			auto result = func(m_resInst);
			::LeaveCriticalSection(&m_lock);
			return result;
		}
	}


private:
	T m_resInst;
	CRITICAL_SECTION m_lock;
};



}  // namespace gan
