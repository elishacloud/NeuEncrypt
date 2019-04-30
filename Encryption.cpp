/**
* Copyright (C) 2019 NeuShield Inc.
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

void UpdateEncryptFolder(const wchar_t *path)
{
	wcscpy_s(encryptPath, path);
	SetEncryptFolderText();
}

bool IsFileExist(const wchar_t *fileName)
{
	std::ifstream infile(fileName);
	return infile.good();
}

void ReadDirectory(const std::wstring& name, const std::wstring& ext, wstringvec& v)
{
	std::wstring pattern(name + L"\\*" + ext);

	WIN32_FIND_DATA data;
	HANDLE hFind = FindFirstFile(pattern.c_str(), &data);

	if (hFind != INVALID_HANDLE_VALUE)
	{
		do
		{
			if ((data.dwFileAttributes & (FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_SYSTEM)) == 0)
			{
				v.push_back(data.cFileName);
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

void EncryptSingleFile(const std::wstring& path, const std::wstring& name)
{
	std::wstring fileName(path + L"\\" + name);

	// Open file
	std::fstream myfile;
	myfile.open(fileName.c_str(), std::ios::binary | std::ios::in | std::ios::out);

	// Check if file was opened
	if (!myfile.is_open())
	{
		return;
	}

	// Get file size
	myfile.seekg(0, std::ios::end);
	long long fileSize = myfile.tellg();

	// Check file size value
	if (fileSize < 1 || fileSize >= 1024 * 1024 * 1024)
	{
		return;
	}

	long long fileLoc = 0;
	unsigned char memblock[4 * 1024];

	// Encrypt file 4k at a time
	do
	{
		// Get update size
		int updateSize = (int)min(fileSize - fileLoc, 4 * 1024);

		// Read 4k of the file into memory block
		myfile.seekg(fileLoc);
		myfile.read((char*)memblock, updateSize);

		// Encrypt memory block
		for (int x = 0; x < updateSize; x++)
		{
			memblock[x] = memblock[x] xor rndblock[x];
		}

		// Write encrypted memory back to file
		myfile.seekg(fileLoc);
		myfile.write((char*)memblock, updateSize);

		// Set new file location
		fileLoc += updateSize;

	} while (myfile.tellg() < fileSize);

	// Close file
	myfile.close();
}

void EncryptFiles()
{
	if (MessageBox(hwDlg, std::wstring(L"This will encrypt folder '" + std::wstring(encryptPath) + L"'.\n\nAre you sure you want to encrypt this folder?").c_str(), L"NeuEncrypt Confirmation", MB_OKCANCEL) != IDOK)
	{
		return;
	}

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
		RenameFile(encryptPath, originalName.c_str(), tempName.c_str());
		EncryptSingleFile(encryptPath, tempName.c_str());
		RenameFile(encryptPath, tempName.c_str(), encryptName.c_str());
	}

	// Update GUI buttons
	SetButtonStatus();
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
		RenameFile(encryptPath, encryptName.c_str(), tempName.c_str());
		EncryptSingleFile(encryptPath, tempName.c_str());
		RenameFile(encryptPath, tempName.c_str(), originalName.c_str());
	}

	// Update GUI buttons
	SetButtonStatus();
}
