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

#include <cstdint>
#include <functional>



namespace gan
{


using IntMemAddr = size_t;  // integral type for memory address


// A wrapper for the ease of casting and offseting
class MemAddr
{
public:
	// Can't use reinterpret_cast as it has undefined behavior and thus incompatible with constexpr.
	constexpr MemAddr() : m_addr(0) { }
	constexpr MemAddr(const void* addr) : m_addr((IntMemAddr)(addr)) { }
	constexpr MemAddr(IntMemAddr addr) : m_addr(addr) { }

	// casting
	constexpr operator const void* () const		{ return (const void*)(m_addr); }
	constexpr operator const uint8_t* () const	{ return (const uint8_t*)(m_addr); }
	constexpr operator IntMemAddr () const		{ return m_addr; }
	constexpr operator void* ()					{ return (void*)(m_addr); }
	constexpr operator uint8_t* ()				{ return (uint8_t*)(m_addr); }

	// more generic casting
	template <typename T> constexpr const T* As() const	{ return (const T*)(m_addr); }
	template <typename T> constexpr T* As()				{ return (T*)(m_addr); }

	// validity
	constexpr bool operator ! () const	{ return m_addr == 0; }
	constexpr bool IsValid() const		{ return m_addr != 0; }

	// comparisons
	constexpr bool operator == (MemAddr other) const	{ return m_addr == other.m_addr; }
	constexpr bool operator != (MemAddr other) const	{ return m_addr != other.m_addr; }
	constexpr bool operator > (MemAddr other) const		{ return m_addr > other.m_addr; }
	constexpr bool operator >= (MemAddr other) const	{ return m_addr >= other.m_addr; }

	// arithmetics
	constexpr MemAddr Offset(intptr_t offset) const			{ return m_addr + offset; }
	constexpr ptrdiff_t operator - (MemAddr other) const	{ return m_addr - other.m_addr; }

private:
	IntMemAddr m_addr;
};


// Only closed at the lower endpoing, i.e., [min, max)
struct MemRange
{
	MemAddr min;
	MemAddr max;

	constexpr bool InRange(MemAddr addr) const
	{
		return addr >= min && max > addr;
	}
};


template <typename T>
class Singleton
{
public:
	static T& GetInstance()
	{
		static T* s_inst = new T;
		return *s_inst;
	}
};


// Windows API types
using WinHandle = void*;
using WinErrorCode = unsigned long;



constexpr MemAddr k_nullptr;


enum
{
	k_64bit = (sizeof(IntMemAddr) == 8)
};

enum class Arch : uint8_t
{
	IA32,
	Amd64
};


}  // namespace gan



// std::hash specializations
namespace std
{

	
template <> struct hash<gan::MemAddr>
{
	std::size_t operator()(gan::MemAddr key) const { return hash<gan::IntMemAddr>()(key); }
};


}  // namespace std
