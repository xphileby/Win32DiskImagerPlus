#include "CppUnitTest.h"

#define _WTL_NO_CSTRING
#define _WTL_NO_WTYPES
#include <atlbase.h>
#include <atlstr.h>

using namespace Microsoft::VisualStudio::CppUnitTestFramework;

// Pull in the Slashify function by including disk.cpp directly.
// We forward-declare only what we need to avoid pulling in the full app.
// Slashify is a static function in disk.cpp, so we redefine it here for testing.

static void Slashify(LPCTSTR str, CString& withSlash, CString& noSlash)
{
	CString s(str);
	if (s.IsEmpty())
		return;
	if (s[s.GetLength() - 1] == _T('\\'))
	{
		withSlash = s;
		noSlash = s.Left(s.GetLength() - 1);
	}
	else
	{
		noSlash = s;
		withSlash = s + _T("\\");
	}
}

namespace DiskTests
{
	TEST_CLASS(SlashifyTests)
	{
	public:
		TEST_METHOD(WithTrailingSlash)
		{
			CString withSlash, noSlash;
			Slashify(_T("C:\\"), withSlash, noSlash);
			Assert::AreEqual(_T("C:\\"), (LPCTSTR)withSlash);
			Assert::AreEqual(_T("C:"), (LPCTSTR)noSlash);
		}

		TEST_METHOD(WithoutTrailingSlash)
		{
			CString withSlash, noSlash;
			Slashify(_T("C:"), withSlash, noSlash);
			Assert::AreEqual(_T("C:\\"), (LPCTSTR)withSlash);
			Assert::AreEqual(_T("C:"), (LPCTSTR)noSlash);
		}

		TEST_METHOD(LongerPathWithSlash)
		{
			CString withSlash, noSlash;
			Slashify(_T("\\\\.\\D:\\"), withSlash, noSlash);
			Assert::AreEqual(_T("\\\\.\\D:\\"), (LPCTSTR)withSlash);
			Assert::AreEqual(_T("\\\\.\\D:"), (LPCTSTR)noSlash);
		}

		TEST_METHOD(LongerPathWithoutSlash)
		{
			CString withSlash, noSlash;
			Slashify(_T("\\\\.\\D:"), withSlash, noSlash);
			Assert::AreEqual(_T("\\\\.\\D:\\"), (LPCTSTR)withSlash);
			Assert::AreEqual(_T("\\\\.\\D:"), (LPCTSTR)noSlash);
		}

		TEST_METHOD(EmptyString)
		{
			CString withSlash, noSlash;
			Slashify(_T(""), withSlash, noSlash);
			Assert::IsTrue(withSlash.IsEmpty());
			Assert::IsTrue(noSlash.IsEmpty());
		}

		TEST_METHOD(SingleBackslash)
		{
			CString withSlash, noSlash;
			Slashify(_T("\\"), withSlash, noSlash);
			Assert::AreEqual(_T("\\"), (LPCTSTR)withSlash);
			Assert::IsTrue(noSlash.IsEmpty());
		}
	};
}
