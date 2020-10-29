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

#include <Mutex.h>

#include <chrono>
#include <thread>



DEFINE_TESTSUITE_START(Mutex)

	DEFINE_TEST_START(ConcurrentAccess)
	{
		gan::ThreadSafeResource<int> sharedInt(1024);

		volatile bool newThreadStarted = false;

		// New thread: sleeps 5 sec and assigns 4096 to "i"
		std::thread newThread( [&sharedInt, &newThreadStarted]() {
			sharedInt.ApplyOperation( [&newThreadStarted](int& i) {
				newThreadStarted = true;
				std::this_thread::sleep_for(std::chrono::milliseconds(65));
				return i = 4096;
			} );
		} );

		while (!newThreadStarted)
			;  // Spin

		const auto startTime = std::chrono::steady_clock::now();
		sharedInt.ApplyOperation( [](int& i) { return i; } );
		const auto endTime = std::chrono::steady_clock::now();
		const std::chrono::duration<float> deltaTime = endTime - startTime;

		// Must NOT use ASSERT or it won't wait for the thread.
		EXPECT(deltaTime.count() > .055f && deltaTime.count() < .075f);  // 65ms +- 10ms

		newThread.join();
	}
	DEFINE_TEST_END

DEFINE_TESTSUITE_END

