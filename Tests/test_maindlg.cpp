#include "CppUnitTest.h"

#define _WTL_NO_CSTRING
#define _WTL_NO_WTYPES
#include <atlbase.h>
#include <atlstr.h>
#include <windows.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// Re-implement testable pure functions from MainDlg.cpp to unit test them
// without pulling in the full WTL/dialog dependency.

static bool EndsWith(LPCTSTR lpszText, LPCTSTR lpszEnds)
{
	size_t nText = _tcslen(lpszText);
	size_t nEnds = _tcslen(lpszEnds);
	if (nText < nEnds) return false;
	return _tcscmp(lpszText + nText - nEnds, lpszEnds) == 0;
}

static char FirstDriveFromMask(ULONG unitmask)
{
	char i;
	for (i = 0; i < 26; ++i)
	{
		if (unitmask & 0x1)
			break;
		unitmask = unitmask >> 1;
	}
	return (i + 'A');
}

template<typename T>
static CString FormatCapacity(const T& value)
{
	CString strText;
	if (value.QuadPart >= 1024 * 1024 * 1024)
	{
		strText.Format(_T("%.2f GB"), (double)value.QuadPart / (1024 * 1024 * 1024));
	}
	else if (value.QuadPart >= 1024 * 1024)
	{
		strText.Format(_T("%.2f MB"), (double)value.QuadPart / (1024 * 1024));
	}
	else if (value.QuadPart >= 1024)
	{
		strText.Format(_T("%.2f KB"), (double)value.QuadPart / 1024);
	}
	else
	{
		strText.Format(_T("%d B"), value.LowPart);
	}
	return strText;
}

namespace MainDlgTests
{
	TEST_CLASS(EndsWithTests)
	{
	public:
		TEST_METHOD(MatchingSuffix)
		{
			Assert::IsTrue(EndsWith(_T("test.img"), _T(".img")));
		}

		TEST_METHOD(NonMatchingSuffix)
		{
			Assert::IsFalse(EndsWith(_T("test.iso"), _T(".img")));
		}

		TEST_METHOD(SuffixLongerThanText)
		{
			Assert::IsFalse(EndsWith(_T(".img"), _T("test.img")));
		}

		TEST_METHOD(ExactMatch)
		{
			Assert::IsTrue(EndsWith(_T(".img"), _T(".img")));
		}

		TEST_METHOD(EmptyString)
		{
			Assert::IsTrue(EndsWith(_T("anything"), _T("")));
		}

		TEST_METHOD(BothEmpty)
		{
			Assert::IsTrue(EndsWith(_T(""), _T("")));
		}

		TEST_METHOD(EmptyTextNonEmptySuffix)
		{
			Assert::IsFalse(EndsWith(_T(""), _T("x")));
		}
	};

	TEST_CLASS(FirstDriveFromMaskTests)
	{
	public:
		TEST_METHOD(DriveA)
		{
			Assert::AreEqual('A', FirstDriveFromMask(0x1));
		}

		TEST_METHOD(DriveC)
		{
			Assert::AreEqual('C', FirstDriveFromMask(0x4));
		}

		TEST_METHOD(DriveZ)
		{
			Assert::AreEqual('Z', FirstDriveFromMask(0x02000000));
		}

		TEST_METHOD(MultipleDrivesReturnsFirst)
		{
			// Mask with D and F set - should return D
			Assert::AreEqual('D', FirstDriveFromMask(0x28));
		}
	};

	TEST_CLASS(FormatCapacityTests)
	{
	public:
		TEST_METHOD(FormatBytes)
		{
			ULARGE_INTEGER val;
			val.QuadPart = 512;
			CString result = FormatCapacity(val);
			Assert::AreEqual(_T("512 B"), (LPCTSTR)result);
		}

		TEST_METHOD(FormatKB)
		{
			ULARGE_INTEGER val;
			val.QuadPart = 2048;
			CString result = FormatCapacity(val);
			Assert::AreEqual(_T("2.00 KB"), (LPCTSTR)result);
		}

		TEST_METHOD(FormatMB)
		{
			ULARGE_INTEGER val;
			val.QuadPart = 1024 * 1024 * 5;
			CString result = FormatCapacity(val);
			Assert::AreEqual(_T("5.00 MB"), (LPCTSTR)result);
		}

		TEST_METHOD(FormatGB)
		{
			ULARGE_INTEGER val;
			val.QuadPart = (ULONGLONG)1024 * 1024 * 1024 * 2;
			CString result = FormatCapacity(val);
			Assert::AreEqual(_T("2.00 GB"), (LPCTSTR)result);
		}

		TEST_METHOD(FormatZero)
		{
			ULARGE_INTEGER val;
			val.QuadPart = 0;
			CString result = FormatCapacity(val);
			Assert::AreEqual(_T("0 B"), (LPCTSTR)result);
		}
	};
}
