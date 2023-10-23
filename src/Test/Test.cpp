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

#include "Test.h"

#include <cassert>
#include <string>
#include <unordered_map>

#include <strsafe.h>
#include <windows.h>


namespace
{


enum class ConsoleColor : uint8_t
{
	Black = 30,
	Red = 31,
	Green = 32,
	Yellow = 33,
	BrightGray = 37,

	DarkGray = Black + 60,
	BrightRed = Red + 60,
	BrightGreen = Green + 60,
	BrightYellow = Yellow + 60,
	White = BrightGray + 60,
};


class ConsoleColorScope
{
public:
	ConsoleColorScope(ConsoleColor color)
	{
		printf("\033[%dm", static_cast<int>(color));
	}

	~ConsoleColorScope()
	{
		printf("\033[0m");
	}
};


class ConsoleColorHelper
{
public:
	static void Print(ConsoleColor color, const char* str)
	{
		ConsoleColorScope colorStr(color);
		printf("%s", str);
	}
};


void EnableTerminalColoring(HANDLE console)
{
	DWORD consoleMode;
	if (GetConsoleMode(console, &consoleMode))
		SetConsoleMode(console, consoleMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
}


}  // unnamed namespace



TestSuite::Result TestSuite::RunAll()
{
	Result result;
	result.numPassed = 0;
	result.numTests = static_cast<unsigned int>(__m_testCaseRegistryList.size());

	std::vector<TestCaseBase*> testsRun;
	std::vector<TestCaseBase*> testsFailedInit;

	// Run all tests
	printf("Suite: %s  ", GetName());
	for (const auto* testCaseReg : __m_testCaseRegistryList)
	{
		assert(testCaseReg);

		auto* testInst = testCaseReg->funcMakeInst();
		if (testInst->SetUp())
		{
			testsRun.emplace_back(testInst);
			testInst->Run();
			testInst->TearDown();

			if (testInst->__m_failedAssertions.empty() && testInst->__m_failedExpectations.empty())
				++result.numPassed;
		}
		else
			testsFailedInit.emplace_back(testInst);
	}

	// Output report

	// Overall result for this test suite
	if (result.numPassed == result.numTests)
		ConsoleColorHelper::Print(ConsoleColor::BrightGreen, "Passed\n");
	else
		ConsoleColorHelper::Print(ConsoleColor::BrightRed, "Failed\n");

	// Test case details (only those which failed)
	for (const auto* failedTest : testsFailedInit)
	{
		printf("    Test: %s  ", failedTest->__m_name);
		ConsoleColorHelper::Print(ConsoleColor::BrightRed, "Init failed\n");
	}
	for (const auto* runTest : testsRun)
	{
		if (runTest->__m_failedAssertions.empty() && runTest->__m_failedExpectations.empty())
		{
			printf("    Test: %s  ", runTest->__m_name);
			ConsoleColorHelper::Print(ConsoleColor::BrightGreen, "Passed\n");
		}

		for (const auto* failedAssertion : runTest->__m_failedAssertions)
		{
			printf("    Test: %s  ", runTest->__m_name);
			ConsoleColorScope color(ConsoleColor::BrightRed);
			printf("Assert \"%s\"\n", failedAssertion);
		}
		for (const auto* failedExpectation : runTest->__m_failedExpectations)
		{
			printf("    Test: %s  ", runTest->__m_name);
			ConsoleColorScope color(ConsoleColor::BrightRed);
			printf("Expect \"%s\"\n", failedExpectation);
		}
	}

	// Clean up test instances
	for (const auto* runTest : testsRun)
		delete runTest;
	for (const auto* initFailedTest : testsFailedInit)
		delete initFailedTest;

	return result;
}


void TestManager::Add(TestSuite* test)
{
	GetInstance().m_tests.emplace_back(test);
}


bool TestManager::RunAll()
{
	EnableTerminalColoring(GetStdHandle(STD_OUTPUT_HANDLE));

	bool hasFailure = false;
	TestManager& instance = GetInstance();

	// Check name conflicts. This can only be done in run time.
	{
		std::unordered_map<std::string, unsigned int> allNames;
		for (const auto& test : instance.m_tests)
			++allNames[test->GetName()];

		for (const auto& nameRecord : allNames)
		{
			if (nameRecord.second > 1)
			{
				printf("Name conflict: %d test suites have the same name ", nameRecord.second);
				ConsoleColorScope color(ConsoleColor::BrightRed);
				printf("\"%s\"\n", nameRecord.first.c_str());

				hasFailure = true;
			}
		}
	}

	if (!hasFailure)
	{
		const auto suitesAll = static_cast<unsigned int>(instance.m_tests.size());
		unsigned int suitesPassed = 0;

		for (auto& test : instance.m_tests)
		{
			TestSuite::Result res = test->RunAll();
			if (res.numTests == res.numPassed)
				++suitesPassed;
			else
				hasFailure = true;
		}

		ConsoleColorHelper::Print(ConsoleColor::White, "\n>=== Test suite results ===\n");
		char bufferNumeric[16];  // Enough for %u
		printf("All: %u\n", suitesAll);

		printf("Passed: ");
		StringCbPrintfA(bufferNumeric, sizeof(bufferNumeric), "%u\n", suitesPassed);
		ConsoleColorHelper::Print(ConsoleColor::BrightGreen, bufferNumeric);

		assert(suitesAll >= suitesPassed);
		const unsigned int suitesFailed = suitesAll - suitesPassed;
		printf("Failed: ");
		StringCbPrintfA(bufferNumeric, sizeof(bufferNumeric), "%u\n", suitesAll - suitesPassed);
		ConsoleColorHelper::Print(
			suitesFailed > 0 ? ConsoleColor::BrightRed : ConsoleColor::BrightGray,
			bufferNumeric
		);
	}

	return !hasFailure;
}
