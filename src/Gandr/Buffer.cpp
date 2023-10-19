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

#include <Buffer.h>
#include <Types.h>

#include <cassert>
#include <cstring>
#include <limits>

#include <intrin.h>
#include <windows.h>



namespace
{



size_t DetermineCapacity(size_t requestedSize)
{
	constexpr size_t maxSize = std::numeric_limits<size_t>::max();
	constexpr size_t maxSizeForReservation = 1 << 26;  // Not to reserve memory if requestedSize >= 64 MB

	if (requestedSize < static_cast<size_t>(gan::Buffer::k_minSize))
		return gan::Buffer::k_minSize;
	else if (requestedSize >= maxSizeForReservation)
		return requestedSize;

#if defined _WIN64
	return 1ull << (64ull - __lzcnt64(requestedSize));
#else
	return 1ul << (32u - __lzcnt(static_cast<unsigned int>(requestedSize)));
#endif  // _WIN64
}



}  // unnamed namespace



namespace gan
{



std::unique_ptr<Buffer> Buffer::Allocate(size_t size)
{
	auto capacity = DetermineCapacity(size);
	assert(capacity >= size);
	if (capacity >= size)
	{
		MemAddr dataPtr{ ::HeapAlloc(GetProcessHeap(), 0, capacity) };
		if (dataPtr.IsValid())
			return std::unique_ptr<Buffer>{ new Buffer{ capacity, size, dataPtr.Ptr<uint8_t>()} };
	}
	return nullptr;
}


Buffer::Buffer(size_t capacity, size_t size, uint8_t* addr)
	: m_capacity(capacity)
	, m_size(size)
	, m_data(addr)
{
	assert(capacity >= size);
	assert(capacity > 0);
	assert(addr != nullptr);
}


Buffer::~Buffer()
{
	::HeapFree(GetProcessHeap(), 0, m_data);
}


bool Buffer::Resize(size_t newSize)
{
	if (newSize < m_capacity)
		return true;

	auto newCapacity = DetermineCapacity(newSize);
	assert(newCapacity >= newSize);
	if (newCapacity < newSize)
		return false;

	MemAddr newAddr{ ::HeapReAlloc(::GetProcessHeap(), 0, m_data, newCapacity) };
	assert(newAddr.IsValid());
	if (newAddr.IsValid())
	{
		m_capacity = newCapacity;
		m_size = newSize;
		m_data = newAddr.Ptr<uint8_t>();

		return true;
	}

	return false;
}


}  // namespace gan
