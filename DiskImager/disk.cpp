/**********************************************************************
 *  This program is free software; you can redistribute it and/or     *
 *  modify it under the terms of the GNU General Public License       *
 *  as published by the Free Software Foundation; either version 2    *
 *  of the License, or (at your option) any later version.            *
 *                                                                    *
 *  This program is distributed in the hope that it will be useful,   *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of    *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the     *
 *  GNU General Public License for more details.                      *
 *                                                                    *
 *  You should have received a copy of the GNU General Public License *
 *  along with this program; if not, see http://gnu.org/licenses/     *
 *  ---                                                               *
 *  Copyright (C) 2009, Justin Davis <tuxdavis@gmail.com>             *
 *  Copyright (C) 2009-2017 ImageWriter developers                    *
 *                 https://sourceforge.net/projects/win32diskimager/  *
 **********************************************************************/

#include "stdafx.h"
#include "resource.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <windows.h>
#include <winioctl.h>
#include <Setupapi.h>
//#include <Ntddstor.h>
#include <cfgmgr32.h>
#include "disk.h"
#include "Logger.h"

#pragma comment( lib, "setupapi.lib" )

void ShowErrorMessage(HWND hWnd, LPCTSTR lpszMessage, LPCTSTR lpszTail=NULL)
{
	LPTSTR errormessage = NULL;
	DWORD dwError = GetLastError();
	::FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER, NULL, dwError, 0,
		(LPWSTR)&errormessage, 0, NULL);
	CString errText;
	errText.Format(_T("%s\nError %d: %s"), lpszMessage, dwError, errormessage);
	if (lpszTail)
		errText.AppendFormat(_T("\n%s"), lpszTail);
	LOGW(L"ERROR: %s", (LPCTSTR)errText);
	AtlMessageBox(hWnd,  (LPCTSTR)errText, IDR_MAINFRAME, MB_ICONERROR | MB_OK);
	LocalFree(errormessage);
}

CAtlFile GetHandleOnFile(HWND hWnd, LPCTSTR filelocation, DWORD access)
{
	LOGW(L"GetHandleOnFile: path='%s', access=0x%X", filelocation, access);
	CAtlFile file;
    if(!SUCCEEDED(file.Create(filelocation, access, (access == GENERIC_READ) ? FILE_SHARE_READ : 0,
		(access == GENERIC_READ) ? OPEN_EXISTING : CREATE_ALWAYS)))
    {
		ShowErrorMessage(hWnd, _T("An error occurred when attempting to get a handle on the file."));
    }
	else
	{
		LOG("GetHandleOnFile: success, handle=0x%p", (HANDLE)file);
	}
    return file;
}

DWORD CVolume::GetDeviceID()
{
    VOLUME_DISK_EXTENTS sd;
    ZeroMemory(&sd, sizeof(sd));
    DWORD bytesreturned;
    LOG("CVolume::GetDeviceID: handle=0x%p", (HANDLE)m_File);
    BOOL bResult = DeviceIoControl(m_File, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, NULL, 0, &sd, sizeof(sd), &bytesreturned, NULL);
    LOG_IOCTL("IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS", (HANDLE)m_File, bResult, bytesreturned);
    if (!bResult)
    {
		ShowErrorMessage(m_hMsgWnd, _T("An error occurred when attempting to get information on volume."));
		return (DWORD)-1;
    }
    LOG("CVolume::GetDeviceID: DiskNumber=%lu", sd.Extents[0].DiskNumber);
    return sd.Extents[0].DiskNumber;
}

CAtlFile GetHandleOnDevice(HWND hWnd, DWORD dwDevice, DWORD access)
{
    CAtlFile DeviceFile;
	CString strDeviceName;
	if(LOWORD(dwDevice)==DEVICE_DISK)
		strDeviceName.Format(_T("\\\\.\\PhysicalDrive%d"), HIWORD(dwDevice));
	else if(LOWORD(dwDevice)==DEVICE_DRIVE)
		strDeviceName.Format(_T("\\\\.\\%c:"), (TCHAR)HIWORD(dwDevice));
	LOGW(L"GetHandleOnDevice: name='%s', access=0x%X", (LPCTSTR)strDeviceName, access);
	if(!SUCCEEDED(DeviceFile.Create(strDeviceName, access, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING)))
    {
		ShowErrorMessage(hWnd, _T("An error occurred when attempting to get a handle on the device."));
    }
	else
	{
		LOG("GetHandleOnDevice: success, handle=0x%p", (HANDLE)DeviceFile);
	}
    return DeviceFile;
}

bool CVolume::Open(HWND hWnd, TCHAR volume, DWORD access)
{
    TCHAR szVolumeName[] = _T("\\\\.\\A:");
	szVolumeName[4] += volume-_T('A');
	m_hMsgWnd = hWnd;
	if(!SUCCEEDED(m_File.Create(szVolumeName, access, FILE_SHARE_READ | FILE_SHARE_WRITE, OPEN_EXISTING)))
    {
		ShowErrorMessage(m_hMsgWnd, _T("An error occurred when attempting to get a handle on the volume."));
		return false;
    }
    return true;
}

bool CVolume::Lock()
{
    DWORD bytesreturned;
    BOOL bResult;
    bResult = DeviceIoControl(m_File, FSCTL_LOCK_VOLUME, NULL, 0, NULL, 0, &bytesreturned, NULL);
	if (bResult)
		m_bLocked = true;
	else
		ShowErrorMessage(m_hMsgWnd, _T("An error occurred when attempting to lock the volume."));
    return (bResult);
}

bool CVolume::Unlock()
{
    DWORD junk;
    BOOL bResult;
    bResult = DeviceIoControl(m_File, FSCTL_UNLOCK_VOLUME, NULL, 0, NULL, 0, &junk, NULL);
	if (bResult)
		m_bLocked = false;
	else
		ShowErrorMessage(m_hMsgWnd, _T("An error occurred when attempting to unlock the volume."));
    return (bResult);
}

bool CVolume::Unmount()
{
    DWORD junk;
    BOOL bResult;
    bResult = DeviceIoControl(m_File, FSCTL_DISMOUNT_VOLUME, NULL, 0, NULL, 0, &junk, NULL);
    if (!bResult)
    {
		ShowErrorMessage(m_hMsgWnd, _T("An error occurred when attempting to dismount the volume."));
    }
    return (bResult);
}

bool CVolume::IsUnmounted()
{
    DWORD junk;
    BOOL bResult;
    bResult = DeviceIoControl(m_File, FSCTL_IS_VOLUME_MOUNTED, NULL, 0, NULL, 0, &junk, NULL);
    return (!bResult);
}

void ReadSectorDataFromHandle(HWND hWnd, HANDLE handle, unsigned long long startsector, unsigned long long numsectors, unsigned long long sectorsize,
	std::vector<char>& SectorData)
{
    unsigned long bytesread;
	SectorData.resize(sectorsize * numsectors);
    LARGE_INTEGER li;
    li.QuadPart = startsector * sectorsize;
    DWORD dwResult = SetFilePointer(handle, li.LowPart, &li.HighPart, FILE_BEGIN);
    if (dwResult == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
    {
		ShowErrorMessage(hWnd, _T("An error occurred when attempting to seek to the correct position for reading."));
		SectorData.clear();
		return;
    }
    if (!ReadFile(handle, SectorData.data(), SectorData.size(), &bytesread, NULL))
    {
		ShowErrorMessage(hWnd, _T("An error occurred when attempting to read data from handle."));
		SectorData.clear();
		return;
    }
    if (bytesread < (sectorsize * numsectors))
    {
		SectorData.resize(bytesread);
    }
}

bool WriteSectorDataToHandle(HWND hWnd, HANDLE handle, char *data, unsigned long long startsector, unsigned long long numsectors, unsigned long long sectorsize)
{
    unsigned long byteswritten;
    BOOL bResult;
    LARGE_INTEGER li;
    li.QuadPart = startsector * sectorsize;
    DWORD dwResult = SetFilePointer(handle, li.LowPart, &li.HighPart, FILE_BEGIN);
    if (dwResult == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
    {
		ShowErrorMessage(hWnd, _T("An error occurred when attempting to seek to the correct position for writing."));
		return false;
    }
    bResult = WriteFile(handle, data, sectorsize * numsectors, &byteswritten, NULL);
    if (!bResult)
    {
		ShowErrorMessage(hWnd, _T("An error occurred when attempting to write data to handle."));
    }
    return (bResult);
}

unsigned long long GetDiskSectors(HWND hWnd, HANDLE handle, unsigned long long *pSectorSize)
{
    DWORD junk;
    DISK_GEOMETRY_EX DiskGeometry;
    BOOL bResult;
    bResult = DeviceIoControl(handle, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &DiskGeometry, sizeof(DiskGeometry), &junk, NULL);
    if (!bResult)
    {
		ShowErrorMessage(hWnd, _T("An error occurred when attempting to get the device's geometry."));
        return 0;
    }
    if (pSectorSize != NULL)
    {
        *pSectorSize = (unsigned long long)DiskGeometry.Geometry.BytesPerSector;
    }
    return (unsigned long long)DiskGeometry.DiskSize.QuadPart / (unsigned long long)DiskGeometry.Geometry.BytesPerSector;
}

unsigned long long GetVolumeSectors(HWND hWnd, TCHAR nVolume, unsigned long long *pSectorSize)
{
	TCHAR szRootPath[4]=_T("C:\\");
	szRootPath[0] = nVolume;
	DWORD dwSectorsPerCluster=0, dwBytesPerSector=0, dwNumberOfFreeClusters=0, dwTotalNumberOfClusters=0;
	if (!GetDiskFreeSpace(szRootPath, &dwSectorsPerCluster, &dwBytesPerSector, &dwNumberOfFreeClusters, &dwTotalNumberOfClusters))
	{
		ShowErrorMessage(hWnd, _T("An error occurred when attempting to get the device's geometry."));
		return 0;
	}
	if (pSectorSize != NULL)
		*pSectorSize = dwBytesPerSector;
	return (unsigned long long)dwSectorsPerCluster * dwTotalNumberOfClusters;
}

unsigned long long GetFileSizeInSectors(HWND hWnd, HANDLE handle, unsigned long long sectorsize)
{
    unsigned long long retVal = 0;
    if (sectorsize) // avoid divide by 0
    {
        LARGE_INTEGER filesize;
        if(GetFileSizeEx(handle, &filesize) == 0)
        {
			ShowErrorMessage(hWnd, _T("An error occurred when attempting to get the device's geometry."));
            retVal = 0;
        }
        else
        {
            retVal = ((unsigned long long)filesize.QuadPart / sectorsize ) + (((unsigned long long)filesize.QuadPart % sectorsize )?1:0);
        }
    }
    return(retVal);
}

ULARGE_INTEGER GetDeviceSize(HWND hWnd, DWORD dwDevice, HANDLE hDevice)
{
	ULARGE_INTEGER llSize = { 0 };
	if (LOWORD(dwDevice) == DEVICE_DRIVE)
	{
		TCHAR szRootPath[4] = _T("C:\\");
		szRootPath[0] = HIWORD(dwDevice);
		if (!GetDiskFreeSpaceEx(szRootPath, NULL, &llSize, NULL))
		{
			CString strMessage;
			strMessage.Format(_T("Failed to get the disk space on drive %s."), szRootPath);
			ShowErrorMessage(hWnd, strMessage);
		}
	}
	else
	{
		DISK_GEOMETRY_EX diskGeo;
		DWORD cbBytesReturned;
		if (DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY_EX, NULL, 0, &diskGeo, sizeof(diskGeo), &cbBytesReturned, NULL))
		{
			memcpy(&llSize, &diskGeo.DiskSize, sizeof(ULARGE_INTEGER));
		}
		else
		{
			CString strMessage;
			strMessage.Format(_T("Failed to get the disk space on hard disk %d."), HIWORD(dwDevice));
			ShowErrorMessage(hWnd, strMessage);
		}
	}
	return llSize;
}

bool SpaceAvailable(HWND hWnd, LPCTSTR location, unsigned long long spaceneeded)
{
    ULARGE_INTEGER freespace;
    BOOL bResult;
    bResult = GetDiskFreeSpaceEx(location, NULL, NULL, &freespace);
    if (!bResult)
    {
		CString strMessage;
		strMessage.Format(_T("Failed to get the free space on drive %s."), location);
		ShowErrorMessage(hWnd, strMessage, _T("Checking of free space will be skipped."));
        return true;
    }
    return (spaceneeded <= freespace.QuadPart);
}

// given a drive letter (ending in a slash), return the label for that drive
// TODO make this more robust by adding input verification
CString getDriveLabel(LPCTSTR drv)
{
    CString retVal;
    int szNameBuf = MAX_PATH + 1;
    LPTSTR nameBuf = retVal.GetBuffer(szNameBuf);
    ::GetVolumeInformation(drv, nameBuf, szNameBuf, NULL,
                                        NULL, NULL, NULL, 0);

    // if malloc fails, nameBuf will be NULL.
    // if GetVolumeInfo fails, nameBuf will contain empty string
    // if all succeeds, nameBuf will contain label
	retVal.ReleaseBuffer();
    return retVal;
}

BOOL GetDisksProperty(HWND hWnd, HANDLE hDevice, PSTORAGE_DEVICE_DESCRIPTOR *ppDevDesc,
                      DEVICE_NUMBER *devInfo)
{
    STORAGE_PROPERTY_QUERY Query;
    DWORD dwOutBytes;
    BOOL bResult;
    DWORD cbBytesReturned;

    LOG("GetDisksProperty: hDevice=0x%p", hDevice);

    ZeroMemory(&Query, sizeof(Query));
    Query.PropertyId = StorageDeviceProperty;
    Query.QueryType = PropertyStandardQuery;

    // First pass: query with minimal header to get the required buffer size.
    // Some drivers return FALSE + ERROR_INSUFFICIENT_BUFFER/ERROR_MORE_DATA
    // but still fill in the Size field, so check Size even on failure.
    STORAGE_DESCRIPTOR_HEADER header = { 0 };
    bResult = ::DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
                &Query, sizeof(Query), &header,
                sizeof(header), &dwOutBytes, NULL);
    DWORD dwErr = GetLastError();
    LOG("GetDisksProperty: header query result=%d, error=%lu, dwOutBytes=%lu, header.Version=%lu, header.Size=%lu",
        bResult, dwErr, dwOutBytes, header.Version, header.Size);
    if (!bResult && dwErr != ERROR_INSUFFICIENT_BUFFER && dwErr != ERROR_MORE_DATA)
    {
        LOG("GetDisksProperty: header query failed with unexpected error %lu, skipping device", dwErr);
        return FALSE;
    }

    // Use the size reported by the driver, with a generous minimum floor.
    // Some drivers don't fill in header.Size on failure, so default to 4KB.
    DWORD dwBufSize = header.Size;
    if (dwBufSize < sizeof(STORAGE_DEVICE_DESCRIPTOR))
        dwBufSize = 4096;
    LOG("GetDisksProperty: allocating buffer, size=%lu", dwBufSize);
    *ppDevDesc = (PSTORAGE_DEVICE_DESCRIPTOR)new BYTE[dwBufSize];
    ZeroMemory(*ppDevDesc, dwBufSize);
    (*ppDevDesc)->Size = dwBufSize;

    // Second pass: query with the correctly-sized buffer.
    // Retry once with a larger buffer if the driver reports insufficient space.
    for (int attempt = 0; attempt < 2; attempt++)
    {
        bResult = ::DeviceIoControl(hDevice, IOCTL_STORAGE_QUERY_PROPERTY,
                    &Query, sizeof(Query), *ppDevDesc,
                    dwBufSize, &dwOutBytes, NULL);
        dwErr = GetLastError();
        LOG("GetDisksProperty: full query attempt %d, bufSize=%lu, result=%d, error=%lu, dwOutBytes=%lu",
            attempt, dwBufSize, bResult, dwErr, dwOutBytes);
        if (bResult)
        {
            LOG("GetDisksProperty: BusType=%d, RemovableMedia=%d, VendorIdOffset=%lu, ProductIdOffset=%lu",
                (*ppDevDesc)->BusType, (*ppDevDesc)->RemovableMedia,
                (*ppDevDesc)->VendorIdOffset, (*ppDevDesc)->ProductIdOffset);
            break;
        }
        if ((dwErr == ERROR_INSUFFICIENT_BUFFER || dwErr == ERROR_MORE_DATA) && attempt == 0)
        {
            dwBufSize *= 2;
            LOG("GetDisksProperty: retrying with doubled buffer, size=%lu", dwBufSize);
            delete[] (BYTE*)*ppDevDesc;
            *ppDevDesc = (PSTORAGE_DEVICE_DESCRIPTOR)new BYTE[dwBufSize];
            ZeroMemory(*ppDevDesc, dwBufSize);
            (*ppDevDesc)->Size = dwBufSize;
            continue;
        }
        LOG("GetDisksProperty: query failed after retries, error=%lu, skipping device", dwErr);
        return FALSE;
    }

    bResult = ::DeviceIoControl(hDevice, IOCTL_STORAGE_GET_DEVICE_NUMBER,
                NULL, 0, devInfo, sizeof(DEVICE_NUMBER), &dwOutBytes, NULL);
    LOG_IOCTL("IOCTL_STORAGE_GET_DEVICE_NUMBER", hDevice, bResult, dwOutBytes);
    if (!bResult)
    {
        LOG("GetDisksProperty: IOCTL_STORAGE_GET_DEVICE_NUMBER failed, skipping device");
        return FALSE;
    }
    LOG("GetDisksProperty: DeviceNumber=%lu, DeviceType=%lu, PartitionNumber=%lu",
        devInfo->DeviceNumber, devInfo->DeviceType, devInfo->PartitionNumber);

    return TRUE;
}

// Some routines fail if there's no trailing slash in a name,
// others fail if there is. This routine takes a name and creates
// two versions: one with a trailing slash and one without.
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

bool GetMediaType(HANDLE hDevice)
{
    DISK_GEOMETRY diskGeo;
    DWORD cbBytesReturned;
    BOOL bResult = DeviceIoControl(hDevice, IOCTL_DISK_GET_DRIVE_GEOMETRY, NULL, 0, &diskGeo, sizeof(diskGeo), &cbBytesReturned, NULL);
    LOG_IOCTL("IOCTL_DISK_GET_DRIVE_GEOMETRY", hDevice, bResult, cbBytesReturned);
    if (bResult)
    {
        LOG("GetMediaType: MediaType=%d (FixedMedia=%d, RemovableMedia=%d)", diskGeo.MediaType, FixedMedia, RemovableMedia);
        if ((diskGeo.MediaType == FixedMedia) || (diskGeo.MediaType == RemovableMedia))
        {
            return true;
        }
    }
    return false;
}

bool CheckDriveType(HWND hWnd, LPCTSTR name, ULONG *pid)
{
    HANDLE hDevice;
    PSTORAGE_DEVICE_DESCRIPTOR pDevDesc = NULL;
    DEVICE_NUMBER deviceInfo;
    bool retVal = false;
    CString nameWithSlash, nameNoSlash;
    int driveType;
    DWORD cbBytesReturned;

    Slashify(name, nameWithSlash, nameNoSlash);
    if (nameWithSlash.IsEmpty())
        return false;

    driveType = GetDriveType(nameWithSlash);
    LOGW(L"CheckDriveType: name='%s', driveType=%d", (LPCTSTR)nameWithSlash, driveType);
    switch( driveType )
    {
    case DRIVE_REMOVABLE: // The media can be removed from the drive.
    case DRIVE_FIXED:     // The media cannot be removed from the drive. Some USB drives report as this.
        hDevice = CreateFile(nameNoSlash, FILE_READ_ATTRIBUTES, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
        LOGW(L"CheckDriveType: CreateFile('%s') = 0x%p, error=%lu", (LPCTSTR)nameNoSlash, hDevice, GetLastError());
        if (hDevice == INVALID_HANDLE_VALUE)
        {
			CString strMessage;
			strMessage.Format(_T("An error occurred when attempting to get a handle on %s."), (LPCTSTR)nameWithSlash);
			ShowErrorMessage(hWnd, strMessage);
        }
        else
        {
            bool bMediaOK = GetMediaType(hDevice);
            bool bPropsOK = false;
            if (bMediaOK)
                bPropsOK = GetDisksProperty(hWnd, hDevice, &pDevDesc, &deviceInfo) && pDevDesc != NULL;
            LOG("CheckDriveType: GetMediaType=%d, GetDisksProperty=%d, pDevDesc=%p", bMediaOK, bPropsOK, pDevDesc);
            if (bPropsOK)
                LOG("CheckDriveType: BusType=%d (USB=%d,Sd=%d,Mmc=%d,Sata=%d)", pDevDesc->BusType, BusTypeUsb, BusTypeSd, BusTypeMmc, BusTypeSata);

            // get the device number if the drive is
            // removable or (fixed AND on the usb bus, SD, or MMC (undefined in XP/mingw))
            if(bMediaOK && bPropsOK &&
                    ( ((driveType == DRIVE_REMOVABLE) && (pDevDesc->BusType != BusTypeSata))
                      || ( (driveType == DRIVE_FIXED) && ((pDevDesc->BusType == BusTypeUsb)
                      || (pDevDesc->BusType == BusTypeSd ) || (pDevDesc->BusType == BusTypeMmc )) ) ) )
            {
                // ensure that the drive is actually accessible
                // multi-card hubs were reporting "removable" even when empty
                if(DeviceIoControl(hDevice, IOCTL_STORAGE_CHECK_VERIFY2, NULL, 0, NULL, 0, &cbBytesReturned, (LPOVERLAPPED) NULL))
                {
                    *pid = deviceInfo.DeviceNumber;
                    retVal = true;
                }
                else
                // IOCTL_STORAGE_CHECK_VERIFY2 fails on some devices under XP/Vista, try the other (slower) method, just in case.
                {
                    CloseHandle(hDevice);
                    hDevice = CreateFile(nameNoSlash, FILE_READ_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
                    if(DeviceIoControl(hDevice, IOCTL_STORAGE_CHECK_VERIFY, NULL, 0, NULL, 0, &cbBytesReturned, (LPOVERLAPPED) NULL))
                    {
                        *pid = deviceInfo.DeviceNumber;
                        retVal = true;
                    }
                }
            }

            delete[] (BYTE*)pDevDesc;
            CloseHandle(hDevice);
        }

        break;
    default:
        retVal = false;
    }

    return(retVal);
}

void ScanDiskDevices(std::vector<CDiskInfo>& disks)
{
	HDEVINFO hDiskClassDevices;
	GUID diskClassDeviceInterfaceGuid = GUID_DEVINTERFACE_DISK;
	SP_DEVICE_INTERFACE_DATA deviceInterfaceData;
	PSP_DEVICE_INTERFACE_DETAIL_DATA deviceInterfaceDetailData = NULL;
	DWORD requiredSize;
	DWORD deviceIndex;

	CHandle disk;
	STORAGE_DEVICE_NUMBER diskNumber;
	DWORD bytesReturned;

	LOG("ScanDiskDevices: starting enumeration");

	hDiskClassDevices = SetupDiGetClassDevs(&diskClassDeviceInterfaceGuid,
		NULL,
		NULL,
		DIGCF_PRESENT |
		DIGCF_DEVICEINTERFACE);
	if (INVALID_HANDLE_VALUE == hDiskClassDevices)
	{
		LOG_ERR("SetupDiGetClassDevs");
		goto Exit;
	}

	ZeroMemory(&deviceInterfaceData, sizeof(SP_DEVICE_INTERFACE_DATA));
	deviceInterfaceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);
	deviceIndex = 0;

	while (SetupDiEnumDeviceInterfaces(hDiskClassDevices,
		NULL,
		&diskClassDeviceInterfaceGuid,
		deviceIndex,
		&deviceInterfaceData))
	{

		++deviceIndex;

		SetupDiGetDeviceInterfaceDetail(hDiskClassDevices,
			&deviceInterfaceData,
			NULL,
			0,
			&requiredSize,
			NULL);
		if (ERROR_INSUFFICIENT_BUFFER != GetLastError())
			goto Exit;

		deviceInterfaceDetailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)malloc(requiredSize);
		if (NULL == deviceInterfaceDetailData)
			goto Exit;

		ZeroMemory(deviceInterfaceDetailData, requiredSize);
		deviceInterfaceDetailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

		if (!SetupDiGetDeviceInterfaceDetail(hDiskClassDevices,
			&deviceInterfaceData,
			deviceInterfaceDetailData,
			requiredSize,
			NULL,
			NULL))
			goto Exit;

		LOGW(L"ScanDiskDevices[%d]: path='%s'", deviceIndex-1, deviceInterfaceDetailData->DevicePath);

		disk.Attach(CreateFile(deviceInterfaceDetailData->DevicePath,
			GENERIC_READ,
			FILE_SHARE_READ | FILE_SHARE_WRITE,
			NULL,
			OPEN_EXISTING,
			FILE_ATTRIBUTE_NORMAL,
			NULL));
		if (INVALID_HANDLE_VALUE == disk)
		{
			LOG("ScanDiskDevices[%d]: CreateFile failed, error=%lu", deviceIndex-1, GetLastError());
			free(deviceInterfaceDetailData);
			deviceInterfaceDetailData = NULL;
			continue;
		}
		LOG("ScanDiskDevices[%d]: handle=0x%p", deviceIndex-1, (HANDLE)disk);

		SP_DEVINFO_DATA DevInfoData = { sizeof(SP_DEVINFO_DATA) };
		DWORD dwPropertyRegDataType=0;
		DWORD dwRemovalPolicy=0;
		TCHAR szFriendName[128]={0};
		if (SetupDiEnumDeviceInfo(hDiskClassDevices, deviceIndex-1, &DevInfoData))
		{
			if(!SetupDiGetDeviceRegistryProperty(hDiskClassDevices, &DevInfoData, SPDRP_REMOVAL_POLICY,
				&dwPropertyRegDataType, (LPBYTE)&dwRemovalPolicy, sizeof(DWORD), NULL))
			{
				LOG("ScanDiskDevices[%d]: SPDRP_REMOVAL_POLICY failed, error=%lu", deviceIndex-1, GetLastError());
				free(deviceInterfaceDetailData);
				deviceInterfaceDetailData = NULL;
				disk.Close();
				continue;
			}
			if(!SetupDiGetDeviceRegistryProperty(hDiskClassDevices, &DevInfoData, SPDRP_FRIENDLYNAME,
				&dwPropertyRegDataType, (LPBYTE)szFriendName, sizeof(szFriendName), NULL))
			{
				LOG("ScanDiskDevices[%d]: SPDRP_FRIENDLYNAME failed, error=%lu", deviceIndex-1, GetLastError());
				free(deviceInterfaceDetailData);
				deviceInterfaceDetailData = NULL;
				disk.Close();
				continue;
			}
		}

		LOGW(L"ScanDiskDevices[%d]: friendlyName='%s', removalPolicy=%lu", deviceIndex-1, szFriendName, dwRemovalPolicy);

		if (dwRemovalPolicy == CM_REMOVAL_POLICY_EXPECT_NO_REMOVAL)
		{
			free(deviceInterfaceDetailData);
			deviceInterfaceDetailData = NULL;
			disk.Close();
			continue;
		}

		BOOL bDevNum = DeviceIoControl(disk,
			IOCTL_STORAGE_GET_DEVICE_NUMBER,
			NULL,
			0,
			&diskNumber,
			sizeof(STORAGE_DEVICE_NUMBER),
			&bytesReturned,
			NULL);
		LOG_IOCTL("IOCTL_STORAGE_GET_DEVICE_NUMBER(scan)", (HANDLE)disk, bDevNum, bytesReturned);
		if (!bDevNum)
		{
			free(deviceInterfaceDetailData);
			deviceInterfaceDetailData = NULL;
			disk.Close();
			continue;
		}

		disk.Close();

		CDiskInfo diskInfo;
		diskInfo.m_strDevicePath = deviceInterfaceDetailData->DevicePath;
		diskInfo.m_nDeviceType = diskNumber.DeviceType;
		diskInfo.m_nDeviceNumber = diskNumber.DeviceNumber;
		diskInfo.m_strFriendlyName=szFriendName;
		LOG("ScanDiskDevices: adding disk #%lu, type=%lu", diskNumber.DeviceNumber, diskNumber.DeviceType);
		disks.emplace_back(std::move(diskInfo));

		free(deviceInterfaceDetailData);
		deviceInterfaceDetailData = NULL;
	}

Exit:
	free(deviceInterfaceDetailData);

	if (INVALID_HANDLE_VALUE != hDiskClassDevices)
		SetupDiDestroyDeviceInfoList(hDiskClassDevices);

	if (INVALID_HANDLE_VALUE != disk)
		disk.Close();

	LOG("ScanDiskDevices: finished, found %zu disk(s)", disks.size());
}
