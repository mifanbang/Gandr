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

#include <memory>


namespace gan
{


class Buffer
{
private:
	// To prevent user code from calling Buffer(size_t, size_t, uint8_t*, Private) directly
	// while making it possible in Allocate() to use std::make_unique
	class Private
	{
		friend class Buffer;
		Private() = default;
	};

public:
	constexpr static size_t k_minSize = 128;  // 128 B

	// Factory function
	static std::unique_ptr<Buffer> Allocate(size_t size);

	Buffer(size_t capacity, size_t size, uint8_t* addr, Private) noexcept;
	~Buffer();

	// Non-copyable & non-movable, since there can be no invalidated buffer
	Buffer(const Buffer&) = delete;
	Buffer(Buffer&&) = delete;
	Buffer& operator=(const Buffer&) = delete;
	Buffer& operator=(Buffer&&) = delete;

	operator const uint8_t*() const noexcept { return m_data; }
	operator uint8_t*() noexcept			 { return m_data; }
	const uint8_t* GetData() const noexcept	 { return m_data; }
	uint8_t* GetData() noexcept				 { return m_data; }

	size_t GetCapacity() const noexcept	{ return m_capacity; }
	size_t GetSize() const noexcept		{ return m_size; }
	bool Resize(size_t newSize) noexcept;


private:
	size_t m_capacity;
	size_t m_size;  // size in use
	uint8_t* m_data;
};


}  // namespace gan
