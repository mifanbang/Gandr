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

#include <cstdint>
#include <cstring>



namespace gan
{



template <unsigned int NumOfBits>
struct Hash
{
	static_assert((NumOfBits & 7) == 0, "NumOfBits not a multiple of 8");
	using Type = Hash<NumOfBits>;
	constexpr static size_t k_numOfBytes = NumOfBits >> 3;


	uint8_t data[k_numOfBytes];

	bool operator==(const Type& other) const
	{
		return memcmp(data, other.data, sizeof(data)) == 0;
	}
};



class Hasher
{
public:
	// generate the SHA256 hash for a given buffer.
	// returns a Windows error code indicating the result of the last internal system call.
	static WinErrorCode GetSHA(const void* data, size_t size, Hash<256>& out);
};



}  // namespace gan

