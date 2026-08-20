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


namespace gan
{

class HandleHelper
{
public:
	template <class HandleType>
	static HandleType Duplicate(HandleType handle) noexcept = delete;

	template <> WinHandle Duplicate<WinHandle>(WinHandle handle) noexcept;
};


template <typename ImplType>
	requires (
		// Contract of being AutoHandle-capable
		requires (ImplType) {
			typename ImplType::RawHandle;
			{ ImplType::Close(typename ImplType::RawHandle{ nullptr }) };
		}
	)
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
		other.m_handle = typename ImplType::RawHandle{ nullptr };
	}
	AutoHandle& operator=(AutoHandle&& other) noexcept
	{
		Invalidate();
		m_handle = other.m_handle;
		other.m_handle = typename ImplType::RawHandle{ nullptr };
		return *this;
	}

	constexpr explicit operator bool() const
	{
		return m_handle != typename ImplType::RawHandle{ nullptr }
			&& m_handle != (typename ImplType::RawHandle)(-1);  // INVALID_HANDLE_VALUE := -1
	}
	constexpr ImplType::RawHandle operator*() const noexcept { return m_handle; }
	ImplType::RawHandle& GetRef() noexcept					 { return m_handle; }

	constexpr bool operator ==(const AutoHandle&) const = default;
	constexpr bool operator ==(ImplType::RawHandle otherRawHandle) const { return m_handle == otherRawHandle; }

	void Invalidate() noexcept
	{
		if (operator bool())
		{
			ImplType::Close(m_handle);
			m_handle = typename ImplType::RawHandle{ nullptr };
		}
	}

private:
	ImplType::RawHandle m_handle{ nullptr };
};


namespace internal
{
	struct _AutoWinHandleImpl
	{
		using RawHandle = WinHandle;
		static void Close(RawHandle handle) noexcept;
	};
	struct _AutoWinModuleImpl
	{
		using RawHandle = WinModule;
		static void Close(RawHandle handle) noexcept;
	};
}
using AutoWinHandle = AutoHandle<internal::_AutoWinHandleImpl>;
using AutoWinModule = AutoHandle<internal::_AutoWinModuleImpl>;

}  // namespace gan
