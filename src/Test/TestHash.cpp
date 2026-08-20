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

#include "Test.h"

#include <Gandr/Hash.hpp>

#include <windows.h>


DEFINE_TESTSUITE_START(Hash)

	DEFINE_TEST_START(SHA256)
	{
		constexpr static char k_text[] =
			"Du gamla, Du fria, Du fjällhöga nord.\n"
			"Du tysta, Du glädjerika sköna!\n"
			"Jag hälsar Dig, vänaste land uppå jord,\n"
			"Din sol, Din himmel, Dina ängder gröna.";

		constexpr static gan::Hash<256> k_digest{ {
			0x2b, 0x52, 0x04, 0xcf, 0x34, 0xe9, 0x25, 0x8b, 0x93, 0xc6, 0x1a, 0x96, 0x70, 0x01, 0xf7, 0xc9,
			0xf9, 0x31, 0x6c, 0x09, 0x78, 0xe1, 0xb0, 0xde, 0x41, 0x3a, 0x2c, 0x50, 0x8a, 0xf1, 0x69, 0x84
		} };

		const auto hashResult = gan::Hasher::GetSHA(
			gan::ConstMemAddr{ k_text },
			sizeof(k_text) - 1  // Don't hash the terminating null char.
		);
		ASSERT(hashResult);
		EXPECT(hashResult.value() == k_digest);
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END
