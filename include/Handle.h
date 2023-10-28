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


class HandleHelper
{
public:
	template <class HandleType>
	static HandleType Duplicate(HandleType handle) = delete;

	template <> static WinHandle Duplicate<WinHandle>(WinHandle handle);
};


template <typename ImplType>
	requires (requires (ImplType) {
		typename ImplType::RawHandle;
		{ ImplType::Close((ImplType::RawHandle)(0)) };
	})
class AutoHandle
{
public:
	AutoHandle() = default;
	constexpr explicit AutoHandle(ImplType::RawHandle handle)
		: m_handle(handle)
	{ }
	~AutoHandle()
	{
		Invalidate();
	}
	AutoHandle& operator =(ImplType::RawHandle handle)
	{
		Invalidate();
		m_handle = handle;
		return *this;
	}

	// Non-copyable
	AutoHandle(const AutoHandle&) = delete;
	AutoHandle& operator=(const AutoHandle&) = delete;

	// Movable
	constexpr AutoHandle(AutoHandle&& other) noexcept
		: m_handle(std::move(other.m_handle))
	{
		other.m_handle = (ImplType::RawHandle)(0);
	}
	AutoHandle& operator=(AutoHandle&& other) noexcept
	{
		Invalidate();
		m_handle = other.m_handle;
		other.m_handle = reinterpret_cast<ImplType::RawHandle>(nullptr);
		return *this;
	}

	constexpr operator bool() const
	{
		return m_handle != (ImplType::RawHandle)(0)
			&& m_handle != (ImplType::RawHandle)(-1);  // INVALID_HANDLE_VALUE := -1
	}
	constexpr ImplType::RawHandle operator*() const	{ return m_handle; }
	ImplType::RawHandle& GetRef() { return m_handle; }

	constexpr bool operator ==(const AutoHandle&) const = default;
	constexpr bool operator ==(ImplType::RawHandle otherRawHandle) const { return m_handle == otherRawHandle; }

	void Invalidate()
	{
		if (operator bool())
		{
			ImplType::Close(m_handle);
			m_handle = reinterpret_cast<ImplType::RawHandle>(nullptr);
		}
	}

	// Support of duplication is optional
	template <class Self>
	auto Duplicate(this Self&& self) { return Self{ HandleHelper::Duplicate<ImplType>(self.m_handle) }; }

private:
	ImplType::RawHandle m_handle;
};


namespace internal
{
	struct AutoWinHandleImpl
	{
		using RawHandle = WinHandle;
		static void Close(RawHandle handle);
	};
}
using AutoWinHandle = AutoHandle<internal::AutoWinHandleImpl>;


}  // namespace gan
