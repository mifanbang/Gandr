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

#include <cstdio>
#include <functional>
#include <memory>
#include <vector>

#include <Types.h>



struct TestCaseBase
{
	const char* __m_name { "Unnamed Test Case" };

	// results
	std::vector<const char*> __m_failedAssertions;
	std::vector<const char*> __m_failedExpectations;

	virtual ~TestCaseBase() = default;
	virtual bool SetUp() { return true; }
	virtual void TearDown() { }
	virtual void Run() = 0;
};


struct TestCaseRegistry
{
	std::function<TestCaseBase* ()> funcMakeInst;
	std::function<void (TestCaseBase*)> funcDeleteInst;

	template <typename T>
	TestCaseRegistry(std::vector<const TestCaseRegistry*>& registryList, T*)
	{
		funcMakeInst = []() { return new T; };
		funcDeleteInst = [](TestCaseBase* test) { delete test; };
		registryList.emplace_back(this);
	}
};


struct TestSuite
{
	virtual const char* GetName() const = 0;

	struct Result
	{
		unsigned int numTests;
		unsigned int numPassed;
	};

	Result RunAll();

	std::vector<const TestCaseRegistry*> __m_testCaseRegistryList;
};


#define CONCAT(x, y)		x##y
#define MAKE_INST_NAME(x)	CONCAT(__s_testSuite_, x)

#define DEFINE_TESTSUITE_START(name)						\
	namespace {												\
		struct TestSuite_##name final : public TestSuite {	\
			template <typename T> struct PrivateTestCaseBase : public TestCaseBase { };	\
			TestSuite_##name() { TestManager::Add(this); }	\
			virtual const char* GetName() const override { return #name; }	\

#define DEFINE_TESTSUITE_END		} MAKE_INST_NAME(__COUNTER__); }


#define DEFINE_TEST_SHARED_START	\
			template <> struct PrivateTestCaseBase<void> : public TestCaseBase {

#define DEFINE_TEST_SHARED_END	};

#define DEFINE_TEST_SETUP		virtual bool SetUp() override
#define DEFINE_TEST_TEARDOWN	virtual void TearDown() override


#define DEFINE_TEST_START(name)	\
			TestCaseRegistry __m_testCaseReg_##name { __m_testCaseRegistryList, (PrivateTestCase_##name*)nullptr };	\
			struct PrivateTestCase_##name : public PrivateTestCaseBase<void> {	\
				PrivateTestCase_##name() { __m_name = #name; }				\
				virtual void Run() override

#define DEFINE_TEST_END		};


#define ASSERT(cond)		\
	{						\
		if (!(cond)) {		\
			__m_failedAssertions.emplace_back(#cond);	\
			return;			\
		}					\
	}

#define EXPECT(cond)		\
	{						\
		if (!(cond)) __m_failedExpectations.emplace_back(#cond);	\
	}



class TestManager : public gan::Singleton<TestManager>
{
public:
	static void Add(TestSuite* test);
	static bool RunAll();

private:
	std::vector<TestSuite*> m_tests;
};

