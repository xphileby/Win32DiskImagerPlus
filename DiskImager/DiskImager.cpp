// Win32DiskManager.cpp : main source file for Win32DiskManager.exe
//

#include "stdafx.h"

#include "resource.h"

#include "MainDlg.h"
#include "Logger.h"
#include "version.h"
#include <gdiplus.h>

CAppModule _Module;

int Run(LPTSTR lpstrCmdLine = NULL, int nCmdShow = SW_SHOWDEFAULT)
{
	CMessageLoop theLoop;
	_Module.AddMessageLoop(&theLoop);

	int nArgs;
	LPWSTR* lpszArgv = CommandLineToArgvW(GetCommandLineW(), &nArgs);

	LPCTSTR lpszImageFile = NULL;
	for (int i = 1; i < nArgs; i++)
	{
		if (_wcsicmp(lpszArgv[i], L"--log") == 0)
		{
			TCHAR szLogPath[MAX_PATH];
			GetModuleFileName(NULL, szLogPath, MAX_PATH);
			PathRemoveFileSpec(szLogPath);
			PathAppend(szLogPath, _T("DiskImager.log"));
			CLogger::Instance().Init(szLogPath);
			LOGW(L"Application starting, version " _T(VERSION_STR) L", log file: %s", szLogPath);
			LOG("Logging enabled, args=%d", nArgs);
			for (int j = 0; j < nArgs; j++)
				LOGW(L"  argv[%d] = \"%s\"", j, lpszArgv[j]);

			OSVERSIONINFO osvi = { sizeof(osvi) };
			GetVersionEx(&osvi);
			LOG("OS: Windows %lu.%lu build %lu", osvi.dwMajorVersion, osvi.dwMinorVersion, osvi.dwBuildNumber);

			CString strMsg;
			strMsg.Format(_T("Logging enabled.\nLog file: %s"), szLogPath);
			AtlMessageBox(NULL, (LPCTSTR)strMsg, _T("Win32 Disk Imager"), MB_OK | MB_ICONINFORMATION);
		}
		else if (lpszImageFile == NULL)
		{
			lpszImageFile = lpszArgv[i];
		}
	}

	CMainDlg dlgMain(lpszImageFile);

	if(dlgMain.Create(NULL) == NULL)
	{
		ATLTRACE(_T("Main dialog creation failed!\n"));
		LOG("Main dialog creation FAILED");
		return 0;
	}

	dlgMain.ShowWindow(nCmdShow);

	LocalFree(lpszArgv);

	int nRet = theLoop.Run();

	_Module.RemoveMessageLoop();
	LOG("Application exiting normally, code=%d", nRet);
	return nRet;
}

int WINAPI _tWinMain(HINSTANCE hInstance, HINSTANCE /*hPrevInstance*/, LPTSTR lpstrCmdLine, int nCmdShow)
{
	HRESULT hRes = ::CoInitialize(NULL);
	ATLASSERT(SUCCEEDED(hRes));

	AtlInitCommonControls(ICC_BAR_CLASSES);	// add flags to support other controls

	ULONG_PTR token;
	Gdiplus::GdiplusStartupInput gdiplusStartupInput;
	Gdiplus::GdiplusStartup(&token, &gdiplusStartupInput, NULL);

	hRes = _Module.Init(NULL, hInstance);
	ATLASSERT(SUCCEEDED(hRes));

	int nRet = Run(lpstrCmdLine, nCmdShow);

	Gdiplus::GdiplusShutdown(token);

	_Module.Term();
	::CoUninitialize();

	LOG("Application shutdown complete");
	return nRet;
}
