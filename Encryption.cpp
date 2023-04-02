/**
* Copyright (C) 2022 NeuShield Inc.
*
* This software is  provided 'as-is', without any express  or implied  warranty. In no event will the
* authors be held liable for any damages arising from the use of this software.
* Permission  is granted  to anyone  to use  this software  for  any  purpose,  including  commercial
* applications, and to alter it and redistribute it freely, subject to the following restrictions:
*
*   1. The origin of this software must not be misrepresented; you must not claim that you  wrote the
*      original  software. If you use this  software  in a product, an  acknowledgment in the product
*      documentation would be appreciated but is not required.
*   2. Altered source versions must  be plainly  marked as such, and  must not be  misrepresented  as
*      being the original software.
*   3. This notice may not be removed or altered from any source distribution.
*/

#include "stdafx.h"
#include <windows.h>
#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <algorithm>
#include <iterator>
#include <shlwapi.h>
#include "NeuEncrypt.h"
#include "Encryption.h"

typedef std::vector<std::wstring> wstringvec;

wchar_t processName[MAX_PATH] = { 0 };
wchar_t processPath[MAX_PATH] = { 0 };
wchar_t encryptPath[MAX_PATH] = { 0 };
const wchar_t encryptExt[] = { L".NEUENCRYPT\0" };

void InitEncrypt()
{
	// Get process path
	GetModuleFileName(nullptr, processPath, MAX_PATH);
	wchar_t* p_pName = wcsrchr(processPath, '\\');
	if (p_pName)
	{
		wcscpy_s(processName, p_pName + 1);
		*p_pName = '\0';
	}
	wcscpy_s(encryptPath, processPath);
}

const wchar_t *GetEncryptFolder()
{
	return encryptPath;
}

void SetEncryptFolder(const wchar_t *path)
{
	wcscpy_s(encryptPath, path);
}

bool IsFileExist(const wchar_t *fileName)
{
	std::ifstream infile(fileName);
	return infile.good();
}

void ReadDirectory(const std::wstring& name, const std::wstring& ext, wstringvec& v)
{
	std::wstring pattern(name + L"\\*");

	WIN32_FIND_DATA data;
	HANDLE hFind = FindFirstFile(pattern.c_str(), &data);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((data.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM)) == 0)
			{
				WCHAR combinedPath[MAX_PATH] = { 0 };

				if (PathCombine(combinedPath, name.c_str(), data.cFileName))
				{
					if (ext.empty() || wcscmp(PathFindExtension(data.cFileName), ext.c_str()) == 0)
					{
						v.push_back(combinedPath);
					}
				}
			}
			else if ((data.cFileName[0] == L'.' && data.cFileName[1] == L'\0') || (data.cFileName[0] == L'.' && data.cFileName[1] == L'.' && data.cFileName[2] == L'\0'))
			{
				continue;
			}
			else if ((data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && ((data.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)) == 0))
			{
				WCHAR combinedPath[MAX_PATH] = { 0 };

				if (PathCombine(combinedPath, name.c_str(), data.cFileName))
				{
					ReadDirectory(combinedPath, ext, v);
				}
			}
		} while (FindNextFile(hFind, &data));

		FindClose(hFind);
	}
}

bool CheckIfEncrypted()
{
	// Get number of encrypted files in the folder
	wstringvec v;
	ReadDirectory(encryptPath, encryptExt, v);
	size_t neuFiles = v.size();
	v.clear();

	// If more than one encrypted file
	if (neuFiles)
	{
		return true;
	}

	return false;
}

void RenameFile(const std::wstring& path, const std::wstring& srcname, const std::wstring& dstname)
{
	std::wstring dstFileName(path + L"\\" + dstname);
	std::wstring srcFileName(path + L"\\" + srcname);

	// Check if source file does not exists or if target file does exist
	if (IsFileExist(dstFileName.c_str()) || !IsFileExist(srcFileName.c_str()))
	{
		return;
	}

	// Rename file
	_wrename(srcFileName.c_str(), dstFileName.c_str());
}

bool EncryptSingleFile(const std::wstring& path, const std::wstring& name)
{
	std::wstring fileName(path + L"\\" + name);

	// Open file
	HANDLE hFile = CreateFile(name.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

	if (hFile == INVALID_HANDLE_VALUE)
	{
		DWORD lastError = GetLastError();

		LPWSTR buffer = NULL;
		FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, lastError, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPWSTR)&buffer, 0, NULL);

		std::wcerr << L"Failed to open file \"" << fileName << L"\". Error code: " << GetLastError() << " " << buffer << std::endl;

		LocalFree(buffer);

		return false;
	}

	// Get file size
	LARGE_INTEGER fileSize = { 0 };
	if (!GetFileSizeEx(hFile, &fileSize))
	{
		std::wcerr << L"Failed to get file size for \"" << fileName << L"\". Error code: " << GetLastError() << std::endl;
		CloseHandle(hFile);
		return false;
	}

	// Check file size value
	if (fileSize.QuadPart < 1 || fileSize.QuadPart >= 1024LL * 1024LL * 1024LL)
	{
		std::wcerr << L"File size is invalid for \"" << fileName << L"\"." << std::endl;
		CloseHandle(hFile);
		return false;
	}

	LARGE_INTEGER fileLoc = { 0 };
	unsigned char memblock[4 * 1024];

	// Encrypt file 4k at a time
	do
	{
		// Get update size
		DWORD updateSize = (DWORD)min(fileSize.QuadPart - fileLoc.QuadPart, 4 * 1024);

		// Read 4k of the file into memory block
		DWORD bytesRead = 0;
		OVERLAPPED overlapped = { 0 };
		overlapped.Offset = fileLoc.LowPart;
		overlapped.OffsetHigh = fileLoc.HighPart;
		if (!ReadFile(hFile, memblock, updateSize, &bytesRead, &overlapped) || bytesRead != updateSize)
		{
			std::wcerr << L"Failed to read file \"" << fileName << L"\". Error code: " << GetLastError() << std::endl;
			CloseHandle(hFile);
			return false;
		}

		// Encrypt memory block
		for (DWORD x = 0; x < updateSize; x++)
		{
			memblock[x] = memblock[x] xor rndblock[x];
		}

		// Write encrypted memory back to file
		DWORD bytesWritten = 0;
		if (!WriteFile(hFile, memblock, updateSize, &bytesWritten, &overlapped) || bytesWritten != updateSize)
		{
			std::wcerr << L"Failed to write file \"" << fileName << L"\". Error code: " << GetLastError() << std::endl;
			CloseHandle(hFile);
			return false;
		}

		// Set new file location
		fileLoc.QuadPart += updateSize;

	} while (fileLoc.QuadPart < fileSize.QuadPart);

	// Close file
	CloseHandle(hFile);

	return true;
}

void EncryptFiles()
{
	const wchar_t tempExt[] = { L".encrypting\0" };

	// Read all files in a folder
	wstringvec v;
	ReadDirectory(encryptPath, L"", v);

	// Read all files in a folder
	for (auto& it : v)
	{
		// Get file names
		std::wstring originalName(it);
		std::wstring tempName(originalName + tempExt);
		std::wstring encryptName(originalName + encryptExt);

		// Rename and encrypt file
		RenameFile(LR"(\\?)", originalName.c_str(), tempName.c_str());
		if (EncryptSingleFile(LR"(\\?)", tempName.c_str()))
		{
			RenameFile(LR"(\\?)", tempName.c_str(), encryptName.c_str());
		}
		else
		{
			RenameFile(LR"(\\?)", tempName.c_str(), originalName.c_str());
		}
	}
}

void DecryptFiles()
{
	const wchar_t tempExt[] = { L".decrypting\0" };

	// Read all files in a folder
	wstringvec v;
	ReadDirectory(encryptPath, encryptExt, v);

	// Read all files in a folder
	for (auto& it : v)
	{
		// Get file names
		std::wstring originalName(it);
		originalName.resize(it.size() - std::wstring(encryptExt).size());
		std::wstring tempName(originalName + tempExt);
		std::wstring encryptName(originalName + encryptExt);

		// Rename and encrypt file
		RenameFile(LR"(\\?)", encryptName.c_str(), tempName.c_str());
		if (EncryptSingleFile(LR"(\\?)", tempName.c_str()))
		{
			RenameFile(LR"(\\?)", tempName.c_str(), originalName.c_str());
		}
		else
		{
			RenameFile(LR"(\\?)", tempName.c_str(), encryptName.c_str());
		}
	}
}
