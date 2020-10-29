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



// forward declaration
extern "C" __declspec(dllimport) int __stdcall CloseHandle(_In_ _Post_ptr_invalid_ gan::WinHandle);



namespace gan
{



template <typename TypeHandle, typename FuncDeleter>
class AutoHandle
{
public:
	AutoHandle(TypeHandle handle, FuncDeleter& deleter)
		: m_handle(handle)
		, m_deleter(deleter)
	{ }
	AutoHandle(TypeHandle handle, FuncDeleter&& deleter)
		: m_handle(handle)
		, m_deleter(deleter)
	{ }

	~AutoHandle()
	{
		m_deleter(m_handle);
	}

	__forceinline operator TypeHandle() const	{ return m_handle; }
	__forceinline TypeHandle& GetRef()			{ return m_handle; }

	// Non-copyable and non-movable by default
	AutoHandle(const AutoHandle&) = delete;
	AutoHandle& operator=(const AutoHandle&) = delete;


private:
	FuncDeleter& m_deleter;
	TypeHandle m_handle;
};


class AutoWinHandle : public AutoHandle<WinHandle, decltype(::CloseHandle)>
{
	using super = AutoHandle<WinHandle, decltype(::CloseHandle)>;

public:
	AutoWinHandle(WinHandle handle);

	static AutoWinHandle Duplicate(WinHandle handle);

	bool IsValid() const;
	operator bool() const;

	// movable
	AutoWinHandle(AutoWinHandle&& other) noexcept;
	AutoWinHandle& operator=(AutoWinHandle&& other) noexcept;
};



}  // namespace gan
