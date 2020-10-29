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

#include "Test.h"

#include <Buffer.h>

#include <limits>



DEFINE_TESTSUITE_START(Buffer)

	DEFINE_TEST_START(CapacityCorrectness)
	{
		const auto buffer1 = gan::Buffer::Allocate(0);
		ASSERT(buffer1);
		EXPECT(buffer1->GetCapacity() == gan::Buffer::k_minSize);
		EXPECT(buffer1->GetSize() == 0);

		const auto buffer2 = gan::Buffer::Allocate(127);
		ASSERT(buffer2);
		EXPECT(buffer2->GetCapacity() == gan::Buffer::k_minSize);
		EXPECT(buffer2->GetSize() == 127);

		const auto buffer3 = gan::Buffer::Allocate(128);
		ASSERT(buffer3);
		EXPECT(buffer3->GetCapacity() == 256);
		EXPECT(buffer3->GetSize() == 128);

		const auto buffer4 = gan::Buffer::Allocate(129);
		ASSERT(buffer4);
		EXPECT(buffer4->GetCapacity() == 256);
		EXPECT(buffer4->GetSize() == 129);

		const auto buffer5 = gan::Buffer::Allocate(255);
		ASSERT(buffer5);
		EXPECT(buffer5->GetCapacity() == 256);
		EXPECT(buffer5->GetSize() == 255);

		const auto buffer6 = gan::Buffer::Allocate(256);
		ASSERT(buffer6);
		EXPECT(buffer6->GetCapacity() == 512);
		EXPECT(buffer6->GetSize() == 256);
	}
	DEFINE_TEST_END

	DEFINE_TEST_START(RequestedBufferTooLarge)
	{
		const auto buffer = gan::Buffer::Allocate((std::numeric_limits<size_t>::max() >> 1) + 1);
		EXPECT(!buffer);
	}
	DEFINE_TEST_END

	DEFINE_TEST_START(Request1MB)
	{
		const auto buffer = gan::Buffer::Allocate(1 << 20);
		ASSERT(buffer);
		EXPECT(buffer->GetCapacity() == 1 << 21);
		EXPECT(buffer->GetSize() == 1 << 20);
	}
	DEFINE_TEST_END

	DEFINE_TEST_START(Request1GB)
	{
		const auto buffer = gan::Buffer::Allocate(1 << 30);
		ASSERT(buffer);
		EXPECT(buffer->GetCapacity() == 1 << 30);  // no reservation for large memory
		EXPECT(buffer->GetSize() == 1 << 30);
	}
	DEFINE_TEST_END

	DEFINE_TEST_START(Resizing)
	{
		constexpr uint64_t k_magicNumber = 0x81778187'07758981ull;
		const auto buffer = gan::Buffer::Allocate(1 << 20);  // 1 MB
		ASSERT(buffer);

		gan::MemAddr bufferData = buffer->GetData();
		*bufferData.As<uint64_t>() = k_magicNumber;

		ASSERT(buffer->Resize(1 << 21));
		EXPECT(buffer->GetCapacity() == 1 << 22);
		EXPECT(buffer->GetSize() == 1 << 21);

		gan::MemAddr bufferDataNew = buffer->GetData();
		EXPECT(bufferData != bufferDataNew);
		EXPECT(*bufferDataNew.As<uint64_t>() == k_magicNumber);
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END

