/*
 *  Gandr - another minimalism library for hacking x86-based Windows
 *  Copyright (C) 2020-2025 Mifan Bang <https://debug.tw>.
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


namespace internal
{

	// Utility for memory address manipulation
	template <MemType Mutability>
	class _MemAddrWrapper
	{
		friend class _MemAddrWrapper<MemType::Mutable>;
		friend class _MemAddrWrapper<MemType::Immutable>;

		constexpr static bool IsMutable = (Mutability == MemType::Mutable);

	public:
		using IntegralType = size_t;  // Integral type for memory address

		constexpr _MemAddrWrapper() = default;
		constexpr _MemAddrWrapper(const _MemAddrWrapper&) = default;

		_MemAddrWrapper(const _MemAddrWrapper<MemType::Mutable>& mut)
			requires !IsMutable
		: _MemAddrWrapper(mut.m_addr)
		{ }
		explicit _MemAddrWrapper(const void* addr) noexcept
			requires !IsMutable
		: _MemAddrWrapper(reinterpret_cast<IntegralType>(addr))
		{ }
		explicit _MemAddrWrapper(void* addr) noexcept
			requires IsMutable
		: _MemAddrWrapper(reinterpret_cast<IntegralType>(addr))
		{ }

		constexpr _MemAddrWrapper& operator=(const _MemAddrWrapper&) = default;
		constexpr _MemAddrWrapper& operator=(_MemAddrWrapper&&) = default;

		// Casting
		template <class S = void>
		const S* ConstPtr() const noexcept
		{
			return reinterpret_cast<const S*>(m_addr);
		}
		template <class S = void>
			requires IsMutable
		S* Ptr() const noexcept
		{
			return reinterpret_cast<S*>(m_addr);
		}
		_MemAddrWrapper<MemType::Mutable> ConstCast() const noexcept  // ConstMemAddr -> MemAddr; use with caution
			requires !IsMutable
		{
			return _MemAddrWrapper<MemType::Mutable>{ reinterpret_cast<void*>(m_addr) };
		}

		// Dereferencing
		template <class S>
		const S& ConstRef() const noexcept
		{
			if constexpr (std::is_function_v<S>)  // Keyword "const" for function would be redundant
				return *reinterpret_cast<S*>(m_addr);
			else
				return *reinterpret_cast<const S*>(m_addr);
		}
		template <class S>
			requires IsMutable
		S& Ref() const noexcept
		{
			return *reinterpret_cast<S*>(m_addr);
		}

		// Validity
		constexpr explicit operator bool() const noexcept
		{
			return m_addr;
		}

		// Comparisons
		template <class OtherT>
			requires (requires (OtherT otherT) { { otherT.m_addr } -> std::equality_comparable_with<IntegralType>; })
		constexpr bool operator==(OtherT other) const noexcept
		{
			return m_addr == other.m_addr;
		}
		template <class OtherT>
			requires (requires (OtherT otherT) { { otherT.m_addr } -> std::three_way_comparable_with<IntegralType>; })
		constexpr std::strong_ordering operator<=>(OtherT other) const noexcept
		{
			return m_addr <=> other.m_addr;
		}

		// Bitwise binary
		_MemAddrWrapper operator&(IntegralType mask) const noexcept
		{
			return _MemAddrWrapper{ m_addr & mask };
		}

		// Arithmetics
		constexpr _MemAddrWrapper Offset(intptr_t offset) const noexcept
		{
			return _MemAddrWrapper{ m_addr + offset };
		}
		constexpr ptrdiff_t operator-(_MemAddrWrapper other) const noexcept
		{
			return m_addr - other.m_addr;
		}
		_MemAddrWrapper& operator++() noexcept
		{
			++m_addr;
			return *this;
		}

	private:
		constexpr explicit _MemAddrWrapper(IntegralType addr) noexcept
			: m_addr(addr)
		{ }

		IntegralType m_addr;
	};

}  // namespace internal
using MemAddr = internal::_MemAddrWrapper<MemType::Mutable>;
using ConstMemAddr = internal::_MemAddrWrapper<MemType::Immutable>;
static_assert(sizeof(MemAddr) == sizeof(MemAddr::IntegralType));
static_assert(sizeof(ConstMemAddr) == sizeof(ConstMemAddr::IntegralType));


// Only closed at the lower endpoint, i.e., [min, max)
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
	static T& GetInstance() noexcept(noexcept(T()))
	{
		static T s_inst;
		return s_inst;
	}

	Singleton() = default;
	~Singleton() = default;

private:
	Singleton(const Singleton&) = delete;
	Singleton(Singleton&&) = delete;
	Singleton& operator=(const Singleton&) = delete;
	Singleton& operator=(Singleton&&) = delete;
};


// Windows API types
using WinHandle = void*;
using WinErrorCode = unsigned long;
using WinDword = unsigned long;


enum class Arch : uint8_t { IA32, Amd64 };

consteval bool Is64() noexcept { return sizeof(MemAddr) == 8; }
consteval Arch BuildArch() noexcept { return Is64() ? Arch::Amd64 : Arch::IA32; }

consteval bool UseStdFormat() noexcept { return false; }  // Whether to enable usage of std::format which can boast executable size


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


template <class Enum, class Storage>
	requires (static_cast<std::underlying_type_t<Enum>>(Enum::_Count) <= (sizeof(Storage) << 3))
class Flags
{
public:
	constexpr Flags() = default;
	constexpr Flags(const Flags&) = default;
	constexpr Flags(Flags&&) = default;

	constexpr explicit Flags(Storage s)
		: m_data{ s }
	{ }
	constexpr explicit Flags(Enum e)
		: m_data{ static_cast<Storage>(1) << static_cast<std::underlying_type_t<Enum>>(e) }
	{ }
	template <class... T>
		requires (std::is_same_v<T, Enum> && ...)
	constexpr Flags(Enum e, T... es)
		: Flags{ Flags{ es... }.Set(e) }
	{ }

	constexpr Flags& operator=(const Flags&) = default;
	constexpr Flags& operator=(Flags&&) = default;
	constexpr bool operator==(const Flags&) const = default;

	constexpr operator Storage() const
	{
		return m_data;
	}

	[[nodiscard]] constexpr Flags Set(Enum e) const
	{
		return Flags{ m_data | Flags{ e }.m_data };
	}
	[[nodiscard]] constexpr Flags Clear(Enum e) const
	{
		return Flags{ m_data & ~Flags{ e }.m_data };
	}
	constexpr bool Has(Enum e) const
	{
		return m_data & Flags{ e }.m_data;
	}

private:
	Storage m_data;
};


}  // namespace gan


// std::hash specializations
namespace std
{

template <>
struct hash<gan::MemAddr>
{
	using ImplType = hash<const void*>;
	std::size_t operator()(gan::MemAddr key) const noexcept { return ImplType{}(key.ConstPtr<>()); }
};

template <>
struct hash<gan::ConstMemAddr>
{
	using ImplType = hash<const void*>;
	std::size_t operator()(gan::ConstMemAddr key) const noexcept { return ImplType{}(key.ConstPtr<>()); }
};

}  // namespace std
