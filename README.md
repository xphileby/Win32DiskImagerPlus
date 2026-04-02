# Win32DiskImagerPlus

Win32DiskImagerPlus is a free, simple tool for Windows that lets you write disk image files (such as `.img` or `.iso`) to USB drives and SD cards, and read them back. If you've ever needed to put a Raspberry Pi operating system onto an SD card, create a full backup of a USB drive, or restore a previously saved image -- this is the tool for the job.

It works on all versions of Windows from XP to Windows 11.

## What can it do?

- **Write an image to a device** -- select an image file on your computer and write it to a USB drive or SD card. Commonly used to flash Raspberry Pi OS, RetroPie, LibreELEC, and other embedded Linux distributions.
- **Read a device to an image** -- create an exact copy of your USB drive or SD card as an image file on your computer. This is useful for making full backups before making changes.
- **Verify** -- after writing, compare the image file against the device byte-for-byte to make sure everything was written correctly.
- **Checksum** -- generate MD5, SHA1, or SHA256 checksums to verify the integrity of your image files (for example, to confirm a download wasn't corrupted).
- **Automatic device detection** -- the app detects when you plug in or remove a USB drive or SD card and updates the device list automatically.
- **Partition-aware reading** -- option to read only the allocated partitions rather than the entire device, saving time and disk space.

## How to use

1. Download `DiskImager.exe` from the [Releases](../../releases) page
2. Right-click the file and select **Run as administrator** (the app requires admin privileges to access raw disk devices)
3. Click the folder icon to browse for your image file (`.img`, `.iso`, etc.)
4. Select your target USB drive or SD card from the **Device** dropdown
5. Click **Write** to flash the image, **Read** to create a backup, or **Verify Only** to check an existing write

**Command-line options:**

```
DiskImager.exe [image-file] [--log]
```

- `image-file` -- optional path to a disk image to open on startup
- `--log` -- enable diagnostic logging to `DiskImager.log` in the application directory (useful for troubleshooting)

## System requirements

- Windows XP SP2 or later (32-bit and 64-bit)
- Administrator privileges
- A removable USB drive or SD card

## About this project

Forked from [znone/Win32DiskImager](https://github.com/znone/Win32DiskImager), which was itself a WTL/ATL rewrite of the original Qt-based [Win32 Disk Imager](https://sourceforge.net/projects/win32diskimager/). Because it uses native Windows APIs instead of Qt, the executable is small and has no external dependencies -- just a single `.exe` file, no installation required.

### Differences from the upstream fork

- Fixed multiple critical bugs: undefined behavior in device enumeration, memory leaks, unchecked I/O seek errors, incorrect handle comparisons, uninitialized variables in write path, broken registry settings loading
- Fixed progress bar overflow on devices larger than ~512 MB
- Silently skips drives that don't support storage property queries (virtual disk devices like iODD no longer trigger error dialogs)
- Replaced C-style manual memory management with RAII
- Fixed command-line argument parsing
- Added `--log` command-line option for diagnostic logging
- Added version tracking
- Added unit test project (22 tests)
- Fixed solution file to work with Visual Studio 2022

## Build from source

### Requirements

- Visual Studio 2022 (v143 toolset)
- WTL 10.0.10320 (installed automatically via NuGet)

### Build

```
nuget restore Win32DiskImager.sln
msbuild Win32DiskImager.sln /p:Configuration=Release /p:Platform=Win32
```

Output: `Release\DiskImager.exe`

### Run tests

```
msbuild Win32DiskImager.sln /p:Configuration=Debug /p:Platform=Win32 /t:Tests
vstest.console.exe Debug\Tests\Tests.dll /Platform:x86
```

## License

GNU General Public License v2. See [LICENSE](LICENSE) for the full text.

Originally developed by Justin Davis. Maintained by the [ImageWriter developers](https://sourceforge.net/projects/win32diskimager/).
