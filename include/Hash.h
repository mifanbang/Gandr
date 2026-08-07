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

#include <Types.h>

#include <array>
#include <cstddef>
#include <expected>


namespace gan
{


template <size_t NumOfBits>
	requires ((NumOfBits & 7) == 0)  // Must be a multiple of 8
struct Hash : public std::array<uint8_t, (NumOfBits >> 3)>
{
};

class Hasher
{
public:
	// Generate the SHA256 hash for a given buffer. On error, return a Windows error
	// code of the last failed operation.
	static std::expected<Hash<256>, WinErrorCode> GetSHA(ConstMemAddr dataAddr, size_t size);
};


}  // namespace gan
