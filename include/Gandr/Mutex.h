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

#include <shared_mutex>
#include <type_traits>


namespace gan
{

template <class T>
class ThreadSafeResource
{
public:
	template <class... Arg>
	ThreadSafeResource(Arg&&... arg) noexcept(noexcept(T(std::forward<Arg>(arg)...)))
		: m_resInst(std::forward<Arg>(arg)...)
	{ }
	~ThreadSafeResource() = default;

	ThreadSafeResource(const ThreadSafeResource&) = delete;
	ThreadSafeResource(ThreadSafeResource&&) = delete;
	ThreadSafeResource& operator=(const ThreadSafeResource&) = delete;
	ThreadSafeResource& operator=(ThreadSafeResource&&) = delete;

	// Require the passed in invocable taking T* instead of T& to avoid unintentional copies
	template <class F>
		requires std::invocable<F, T*>
	decltype(auto) Do(const F& func) noexcept(noexcept(func(&m_resInst)))
	{
		std::unique_lock lock(m_mutex);
		return func(&m_resInst);
	}
	template <class F>
		requires std::invocable<F, const T*>
	decltype(auto) DoConst(const F& func) const noexcept(noexcept(func(&m_resInst)))
	{
		std::shared_lock lock(m_mutex);
		return func(&m_resInst);
	}

private:
	T m_resInst;
	mutable std::shared_mutex m_mutex;
};

}  // namespace gan
