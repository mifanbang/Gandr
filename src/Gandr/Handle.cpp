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

#include <Handle.h>

#include <windows.h>


namespace gan
{



AutoWinHandle AutoWinHandle::Duplicate(WinHandle handle)
{
	constexpr uint32_t k_ignoredParam = 0;
	constexpr BOOL k_newHandleInheritable = TRUE;

	HANDLE newHandle;
	HANDLE currProcess = GetCurrentProcess();
	const auto dupResult = ::DuplicateHandle(currProcess, handle, currProcess, &newHandle, k_ignoredParam, k_newHandleInheritable, DUPLICATE_SAME_ACCESS);

	return dupResult != FALSE ? AutoWinHandle(newHandle) : nullptr;
}


AutoWinHandle::AutoWinHandle(WinHandle handle)
	: super(handle, ::CloseHandle)
{
}


bool AutoWinHandle::IsValid() const
{
	return static_cast<bool>(*this);
}


AutoWinHandle::operator bool() const
{
	const WinHandle handle = operator WinHandle();
	return handle != nullptr && handle != INVALID_HANDLE_VALUE;
}


AutoWinHandle::AutoWinHandle(AutoWinHandle&& other) noexcept
	: super(other.GetRef(), ::CloseHandle)
{
	other.GetRef() = nullptr;
}


AutoWinHandle& AutoWinHandle::operator=(AutoWinHandle&& other) noexcept
{
	::CloseHandle(GetRef());
	GetRef() = other.GetRef();
	other.GetRef() = nullptr;
	return *this;
}



}  // namespace gan
