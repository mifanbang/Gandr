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

#if !defined(NOMINMAX) || defined(min) || defined(max)
	#error Macro NOMINMAX is required for the library
#endif  // windows.h NOMINMAX check

#include <functional>
#include <type_traits>


namespace gan
{


enum class MemType { Mutable, Immutable };

namespace internal {
	template <MemType Mutability> class _MemAddrWrapper;
}
using MemAddr = internal::_MemAddrWrapper<MemType::Mutable>;
using ConstMemAddr = internal::_MemAddrWrapper<MemType::Immutable>;


// A wrapper for the ease of casting and offseting memory addresses
template <MemType Mutability>
class internal::_MemAddrWrapper
{
	constexpr static bool IsImmutable = Mutability == MemType::Immutable;

public:
	using IntegralType = size_t;  // Integral type for memory address

	constexpr _MemAddrWrapper() = default;
	constexpr _MemAddrWrapper(const _MemAddrWrapper& other) = default;
	explicit _MemAddrWrapper(const void* addr) requires IsImmutable
		: m_addr(reinterpret_cast<IntegralType>(addr)) { }
	explicit _MemAddrWrapper(void* addr) requires !IsImmutable
		: m_addr(reinterpret_cast<IntegralType>(addr)) { }

	constexpr _MemAddrWrapper& operator=(const _MemAddrWrapper& other) = default;

	// Casting
	template <class S = void>
	const S* ConstPtr() const
	{
		return reinterpret_cast<const S*>(m_addr);
	}
	template <class S = void>
	S* Ptr() const
		requires !IsImmutable
	{
		return reinterpret_cast<S*>(m_addr);
	}
	ConstMemAddr Immutable() const  // MemAddr -> ConstMemAddr
		requires !IsImmutable
	{
		return ConstMemAddr{ reinterpret_cast<const void*>(m_addr) };
	}
	MemAddr ConstCast() const  // ConstMemAddr -> MemAddr; use with caution
		requires IsImmutable
	{
		return MemAddr{ reinterpret_cast<void*>(m_addr) };
	}

	// Dereferencing
	template <class S>
	const S& ConstRef() const
	{
		if constexpr (std::is_function_v<S>)  // keyword "const" for function would be redundant
			return *reinterpret_cast<S*>(m_addr);
		else
			return *reinterpret_cast<const S*>(m_addr);
	}
	template <class S>
	S& Ref() const
		requires !IsImmutable
	{
		return *reinterpret_cast<S*>(m_addr);
	}

	// Validity
	constexpr explicit operator bool() const { return m_addr; }
	constexpr bool IsValid() const			 { return m_addr; }

	// Comparisons
	template <class OtherT>
	constexpr bool operator==(OtherT other) const noexcept
		requires (requires (OtherT otherT) { {otherT.m_addr} -> std::equality_comparable_with<IntegralType>; })
	{
		return m_addr == other.m_addr;
	}
	template <class OtherT>
	constexpr std::strong_ordering operator<=>(OtherT other) const noexcept
		requires (requires (OtherT otherT) { {otherT.m_addr} -> std::three_way_comparable_with<IntegralType>; })
	{
		return m_addr <=> other.m_addr;
	}

	// Bitwise binary
	_MemAddrWrapper operator&(IntegralType mask) const { return _MemAddrWrapper{ m_addr & mask }; }

	// Arithmetics
	_MemAddrWrapper Offset(intptr_t offset) const	{ return _MemAddrWrapper{ m_addr + offset }; }
	ptrdiff_t operator-(_MemAddrWrapper other) const	{ return m_addr - other.m_addr; }
	_MemAddrWrapper& operator++()
	{
		++m_addr;
		return *this;
	}

private:
	explicit _MemAddrWrapper(IntegralType addr) : m_addr(addr) { }

	IntegralType m_addr;
};
static_assert(sizeof(MemAddr) == sizeof(size_t));


// Only closed at the lower endpoing, i.e., [min, max)
template <class T>
struct Range
{
	T min;
	T max;

	template <class OtherT>
	constexpr bool InRange(OtherT addr) const { return addr >= min && max > addr; }
};

using MemRange = Range<MemAddr>;
using ConstMemRange = Range<ConstMemAddr>;


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


enum class Arch : uint8_t { IA32, Amd64 };

consteval bool Is64() { return sizeof(MemAddr) == 8; }
consteval Arch BuildArch() { return Is64() ? Arch::Amd64 : Arch::IA32; }

consteval bool UseStdFormat() { return false; }  // Whether to enable the use of std::format which can boast executable size


// Generalized concept to cover pointers of:
//     1. Non-member functions
//     2. Static member functions
//     3. Non-static member functions
template <class F>
concept IsAnyFuncPtr =
	std::is_member_function_pointer_v<F>
	|| (std::is_pointer_v<F> && std::is_function_v<std::remove_pointer_t<F>>);


namespace internal
{
	// Helper data structure for casting between functions and raw pointers.
	template <class F>
	union _MemFnAddr
	{
		F func;
		void* addr;
	};
}  // namespace internal

// ---------------------------------------------------------------------------
// Function ToMemFn & FromMemFn:
//     Low-level and therefore unsafe casting between a void* raw pointer and
//     a non-static member function pointer.
// ---------------------------------------------------------------------------

template <class F>
	requires std::is_member_function_pointer_v<F>
constexpr F ToMemFn(void* addr)
{
	return internal::_MemFnAddr<F>{ .addr = addr }.func;
}

template <class F>
	requires std::is_member_function_pointer_v<F>
constexpr void* FromMemFn(F func)
{
	return internal::_MemFnAddr<F>{ .func = func }.addr;
}

// ---------------------------------------------------------------------------
// Function ToAnyFn & FromAnyFn:
//     Generalized versions of ToMemFn and FromMemFn
// ---------------------------------------------------------------------------

template <class F>
	requires IsAnyFuncPtr<F>
constexpr static F ToAnyFn(void* addr)
{
	return internal::_MemFnAddr<F>{ .addr = addr }.func;
}

template <class F>
	requires IsAnyFuncPtr<F>
constexpr static void* FromAnyFn(F func)
{
	return internal::_MemFnAddr<F>{ .func = func }.addr;
}


}  // namespace gan


// std::hash specializations
namespace std
{

template <>
struct hash<gan::MemAddr>
{
	using ImplType = hash<const void*>;
	std::size_t operator()(gan::MemAddr key) const { return ImplType{}(key.ConstPtr<>()); }
};

template <>
struct hash<gan::ConstMemAddr>
{
	using ImplType = hash<const void*>;
	std::size_t operator()(gan::ConstMemAddr key) const { return ImplType{}(key.ConstPtr<>()); }
};

}  // namespace std
