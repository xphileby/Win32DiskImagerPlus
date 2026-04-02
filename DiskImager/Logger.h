#pragma once

#include <windows.h>
#include <stdio.h>
#include <stdarg.h>

class CLogger
{
public:
	static CLogger& Instance()
	{
		static CLogger s_instance;
		return s_instance;
	}

	bool Init(LPCTSTR lpszLogPath)
	{
		if (m_hFile != INVALID_HANDLE_VALUE)
			return true;
		m_hFile = CreateFile(lpszLogPath, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
		if (m_hFile == INVALID_HANDLE_VALUE)
			return false;
		// Write UTF-8 BOM
		BYTE bom[] = { 0xEF, 0xBB, 0xBF };
		DWORD written;
		WriteFile(m_hFile, bom, sizeof(bom), &written, NULL);
		Log("=== Win32DiskImagerPlus log started ===");
		return true;
	}

	bool IsEnabled() const { return m_hFile != INVALID_HANDLE_VALUE; }

	void Log(const char* fmt, ...)
	{
		if (m_hFile == INVALID_HANDLE_VALUE)
			return;
		SYSTEMTIME st;
		GetLocalTime(&st);
		char prefix[64];
		sprintf_s(prefix, "[%02d:%02d:%02d.%03d] ",
			st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);

		char buf[2048];
		va_list args;
		va_start(args, fmt);
		int len = vsprintf_s(buf, fmt, args);
		va_end(args);

		EnterCriticalSection(&m_cs);
		DWORD written;
		WriteFile(m_hFile, prefix, (DWORD)strlen(prefix), &written, NULL);
		WriteFile(m_hFile, buf, (DWORD)len, &written, NULL);
		WriteFile(m_hFile, "\r\n", 2, &written, NULL);
		FlushFileBuffers(m_hFile);
		LeaveCriticalSection(&m_cs);
	}

	void LogW(const wchar_t* fmt, ...)
	{
		if (m_hFile == INVALID_HANDLE_VALUE)
			return;
		wchar_t wbuf[2048];
		va_list args;
		va_start(args, fmt);
		vswprintf_s(wbuf, fmt, args);
		va_end(args);

		// Convert to UTF-8
		char buf[4096];
		int len = WideCharToMultiByte(CP_UTF8, 0, wbuf, -1, buf, sizeof(buf), NULL, NULL);
		if (len > 0)
		{
			buf[len - 1] = 0; // remove null terminator from count
			Log("%s", buf);
		}
	}

	void LogLastError(const char* context)
	{
		DWORD dwErr = GetLastError();
		Log("%s failed, error=%lu (0x%08X)", context, dwErr, dwErr);
	}

	void LogDeviceIoControl(const char* ioctlName, HANDLE hDevice, BOOL result, DWORD outBytes)
	{
		if (result)
			Log("DeviceIoControl(%s, handle=0x%p) succeeded, %lu bytes returned", ioctlName, hDevice, outBytes);
		else
		{
			DWORD dwErr = GetLastError();
			Log("DeviceIoControl(%s, handle=0x%p) FAILED, error=%lu (0x%08X)", ioctlName, hDevice, dwErr, dwErr);
			SetLastError(dwErr);
		}
	}

	~CLogger()
	{
		if (m_hFile != INVALID_HANDLE_VALUE)
		{
			Log("=== Log ended ===");
			CloseHandle(m_hFile);
		}
		DeleteCriticalSection(&m_cs);
	}

private:
	CLogger() : m_hFile(INVALID_HANDLE_VALUE)
	{
		InitializeCriticalSection(&m_cs);
	}
	CLogger(const CLogger&);
	CLogger& operator=(const CLogger&);

	HANDLE m_hFile;
	CRITICAL_SECTION m_cs;
};

#define LOG_ENABLED()   CLogger::Instance().IsEnabled()
#define LOG(fmt, ...)   do { if (LOG_ENABLED()) CLogger::Instance().Log(fmt, ##__VA_ARGS__); } while(0)
#define LOGW(fmt, ...)  do { if (LOG_ENABLED()) CLogger::Instance().LogW(fmt, ##__VA_ARGS__); } while(0)
#define LOG_ERR(ctx)    do { if (LOG_ENABLED()) CLogger::Instance().LogLastError(ctx); } while(0)
#define LOG_IOCTL(name, hDev, result, bytes) do { if (LOG_ENABLED()) CLogger::Instance().LogDeviceIoControl(name, hDev, result, bytes); } while(0)
