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
#include <atlbase.h>
#include <shobjidl.h>
#include <shellapi.h>
#include <string>
#include "Encryption.h"
#include "NeuEncrypt.h"

#define MAX_LOADSTRING 100

// Global Variables:
HINSTANCE hInst;                                // current instance
HWND hwDlg;										// current dialog handle
WCHAR szTitle[MAX_LOADSTRING];                  // The title bar text
WCHAR szWindowClass[MAX_LOADSTRING];            // the main window class name

bool RunSilently = false;
wchar_t OverridePath[MAX_PATH] = {};

// Forward declarations of functions included in this code module:
INT_PTR CALLBACK    NeuBox(HWND, UINT, WPARAM, LPARAM);
void GetArgumentOptions(LPCWSTR lpCmdLine);
void SetEncryptFolderText();

int APIENTRY wWinMain(_In_ HINSTANCE hInstance,
                     _In_opt_ HINSTANCE hPrevInstance,
                     _In_ LPWSTR    lpCmdLine,
                     _In_ int       nCmdShow)
{
    UNREFERENCED_PARAMETER(hPrevInstance);
	UNREFERENCED_PARAMETER(nCmdShow);

	// Get command line arguments
	GetArgumentOptions(lpCmdLine);

	// Check if running silently
	if (RunSilently)
	{
		if (wcslen(OverridePath))
		{
			// Set folder to encrypt
			SetEncryptFolder(OverridePath);
			// Do encrypt
			EncryptFiles();
			// return
			return TRUE;
		}
		return FALSE;
	}

    // Initialize global strings
    LoadStringW(hInstance, IDS_APP_TITLE, szTitle, MAX_LOADSTRING);
    LoadStringW(hInstance, IDC_NEUENCRYPT, szWindowClass, MAX_LOADSTRING);
	hInst = hInstance;

    // Perform application initialization:
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_NEUBOX), nullptr, NeuBox);

    return TRUE;
}

void GetArgumentOptions(LPCWSTR lpCmdLine)
{
	// Error checking
	if (!lpCmdLine || wcslen(lpCmdLine) == 0)
	{
		return;
	}

	// Retrieve command line arguments
	LPWSTR* szArgList;
	int argCount;
	szArgList = CommandLineToArgvW(lpCmdLine, &argCount);

	// If arguments
	if (szArgList && argCount)
	{
		for (int i = 0; i < argCount; i++)
		{
			if (_wcsicmp(szArgList[i], L"/silent") == 0 || _wcsicmp(szArgList[i], L"/s") == 0 ||
				_wcsicmp(szArgList[i], L"-silent") == 0 || _wcsicmp(szArgList[i], L"-s") == 0)
			{
				RunSilently = true;
			}
			else if (PathFileExists(szArgList[i]))
			{
				wcscpy_s(OverridePath, MAX_PATH, szArgList[i]);
			}
		}
	}
	LocalFree(szArgList);
}

BOOL CenterWindow(HWND hwndWindow)
{
	if (!IsWindow(hwndWindow))
	{
		return FALSE;
	}

	RECT rectWindow;

	GetWindowRect(hwndWindow, &rectWindow);

	int nWidth = rectWindow.right - rectWindow.left;
	int nHeight = rectWindow.bottom - rectWindow.top;

	int nScreenWidth = GetSystemMetrics(SM_CXSCREEN);
	int nScreenHeight = GetSystemMetrics(SM_CYSCREEN);

	int nX = (nScreenWidth - nWidth) / 2;
	int nY = (nScreenHeight - nHeight) / 2;

	// make sure that the dialog box never moves outside of the screen
	if (nX < 0) nX = 0;
	if (nY < 0) nY = 0;
	if (nX + nWidth > nScreenWidth) nX = nScreenWidth - nWidth;
	if (nY + nHeight > nScreenHeight) nY = nScreenHeight - nHeight;

	SetWindowPos(hwndWindow, NULL, nX, nY, nWidth, nHeight, SWP_NOSIZE | SWP_NOZORDER);

	return TRUE;
}

bool WriteRegistryStruct(const std::wstring& lpzSection, const std::wstring& lpzKey, const LPVOID lpStruct, UINT uSizeStruct)
{
	HKEY hKey;
	std::wstring SubKey(L"SOFTWARE\\" + lpzSection);

	HRESULT hr = RegCreateKey(HKEY_CURRENT_USER, SubKey.c_str(), &hKey);

	if (FAILED(hr))
	{
		return false;
	}

	hr = RegSetValueEx(hKey, lpzKey.c_str(), 0, REG_BINARY, (BYTE*)lpStruct, uSizeStruct);

	if (FAILED(hr))
	{
		return false;
	}

	RegCloseKey(hKey);

	return true;
}

bool ReadRegistryStruct(const std::wstring& lpzSection, const std::wstring& lpzKey, const LPVOID lpStruct, UINT uSizeStruct)
{
	HKEY hKey;
	std::wstring SubKey(L"SOFTWARE\\" + lpzSection);

	HRESULT hr = RegOpenKey(HKEY_CURRENT_USER, SubKey.c_str(), &hKey);

	if (FAILED(hr))
	{
		return false;
	}

	DWORD DataSize = 0;
	hr = RegQueryValueEx(hKey, lpzKey.c_str(), 0, 0, 0, &DataSize);
	if (DataSize > uSizeStruct)
	{
		return false;
	}

	hr = RegQueryValueEx(hKey, lpzKey.c_str(), 0, 0, (BYTE*)lpStruct, &DataSize);

	if (FAILED(hr) || DataSize == 0)
	{
		return false;
	}

	RegCloseKey(hKey);

	return true;
}

void LoadSettings()
{
	WINDOWPLACEMENT wndpl;
	if (ReadRegistryStruct(L"NeuEncrypt", L"WindowPlacement", &wndpl, sizeof(WINDOWPLACEMENT)))
	{
		wndpl.length = sizeof(WINDOWPLACEMENT);
		SetWindowPlacement(hwDlg, &wndpl);
	}

	if (wcslen(OverridePath))
	{
		SetEncryptFolder(OverridePath);
		SetEncryptFolderText();
	}
	else
	{
		wchar_t path[MAX_PATH];
		if (ReadRegistryStruct(L"NeuEncrypt", L"LastEncryptFolder", (LPVOID)path, MAX_PATH * sizeof(wchar_t)))
		{
			SetEncryptFolder(path);
			SetEncryptFolderText();
		}
	}
}

void SaveSettings()
{
	WINDOWPLACEMENT wndpl;
	wndpl.length = sizeof(WINDOWPLACEMENT);
	GetWindowPlacement(hwDlg, &wndpl);

	WriteRegistryStruct(L"NeuEncrypt", L"WindowPlacement", &wndpl, sizeof(WINDOWPLACEMENT));

	WriteRegistryStruct(L"NeuEncrypt", L"LastEncryptFolder", (LPVOID)GetEncryptFolder(), (wcslen(GetEncryptFolder()) + 1) * sizeof(wchar_t));
}

void SetEncryptFolderText()
{
	SetWindowText(GetDlgItem(hwDlg, IDC_ENCPATH), GetEncryptFolder());
}

void SetButtonStatus()
{
	if (CheckIfEncrypted())
	{
		EnableWindow(GetDlgItem(hwDlg, IDENCRYPT), FALSE);
		EnableWindow(GetDlgItem(hwDlg, IDDECRYPT), TRUE);
	}
	else
	{
		EnableWindow(GetDlgItem(hwDlg, IDDECRYPT), FALSE);
		EnableWindow(GetDlgItem(hwDlg, IDENCRYPT), TRUE);
	}
}

void OpenDialog()
{
	CComPtr<IFileDialog> pfd = nullptr;
	WCHAR *folder_name = nullptr;
	CoInitialize(nullptr);

	do
	{
		HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog,
			NULL,
			CLSCTX_INPROC_SERVER,
			IID_PPV_ARGS(&pfd));

		if (FAILED(hr))
		{
			break;
		}

		DWORD dwOptions = 0;
		hr = pfd->GetOptions(&dwOptions);

		if (FAILED(hr))
		{
			break;
		}

		pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);
		pfd->SetTitle(L"Select folders to encrypt");

		hr = pfd->Show(hwDlg);

		if (FAILED(hr))
		{
			break;
		}

		CComPtr<IShellItem> psi = nullptr;

		hr = pfd->GetResult(&psi);

		if (FAILED(hr))
		{
			break;
		}

		hr = psi->GetDisplayName(SIGDN_FILESYSPATH, &folder_name);

		if (FAILED(hr))
		{
			break;
		}

	} while (FALSE);

	if (folder_name)
	{
		SetEncryptFolder(folder_name);
		SetEncryptFolderText();
		SetButtonStatus();
		CoTaskMemFree(folder_name);
		folder_name = nullptr;
	}

	CoUninitialize();
}

void InitDialog()
{
	HICON hCustomIcon = LoadIcon(hInst, MAKEINTRESOURCE(IDI_NEUENCRYPT));
	if (hCustomIcon)
	{
		SendMessage(hwDlg, WM_SETICON, ICON_SMALL, (LPARAM)hCustomIcon);
	}
	CenterWindow(hwDlg);
	LoadSettings();
	SetButtonStatus();
	SetEncryptFolderText();
}

// Message handler for about box.
INT_PTR CALLBACK NeuBox(HWND hDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
    UNREFERENCED_PARAMETER(lParam);
    switch (message)
    {
    case WM_INITDIALOG:
	{
		hwDlg = hDlg;
		InitEncrypt();
		InitDialog();
		return (INT_PTR)TRUE;
	}
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDEXIT:
		case IDCANCEL:
			// Quit program
			SaveSettings();
			EndDialog(hDlg, LOWORD(wParam));
			return (INT_PTR)TRUE;
		case IDBROWSE:
			// Get folder path
			OpenDialog();
			break;
		case IDENCRYPT:
			if (MessageBox(hwDlg, std::wstring(L"This will encrypt folder '" + std::wstring(GetEncryptFolder()) + L"'.\n\nAre you sure you want to encrypt this folder?").c_str(), L"NeuEncrypt Confirmation", MB_OKCANCEL) == IDOK)
			{
				// Do encrypt
				EncryptFiles();
				// Update GUI buttons
				SetButtonStatus();
			}
			break;
		case IDDECRYPT:
			// Do decrypt
			DecryptFiles();
			// Update GUI buttons
			SetButtonStatus();
			break;
		}
        break;
    }
    return (INT_PTR)FALSE;
}
