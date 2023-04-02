//{{NO_DEPENDENCIES}}
// Microsoft Visual C++ generated include file.
// Used by NeuEncrypt.rc
//
#define IDC_MYICON                      2
#define IDD_NEUENCRYPT_DIALOG           102
#define IDS_APP_TITLE                   103
#define IDD_NEUBOX                      103
#define IDM_NEU                         104
#define IDM_EXIT                        105
#define IDI_NEUENCRYPT                  107
#define IDI_NEULOGO                     108
#define IDC_NEUENCRYPT                  109
#define IDR_MAINFRAME                   128
#define IDEXIT                          1000
#define IDDECRYPT                       1001
#define IDENCRYPT                       1002
#define IDBROWSE                        1003
#define IDC_ENCPATH                     1004
#define IDC_HRLINE                      1005
#define IDC_STATIC                      -1

// Main resource file details
#define APP_NAME				"NeuEncrypt"
#define APP_MAJOR				1
#define APP_MINOR				3
#define APP_BUILDNUMBER			0
#define APP_REVISION			0
#define APP_COMPANYNAME			"NeuShield Inc."
#define APP_DESCRPTION			"Simulates ransomware encrypting a folder."
#define APP_COPYRIGHT			"Copyright (C) 2022 NeuShield Inc."
#define APP_ORIGINALVERSION		"NeuEncrypt.exe"
#define APP_INTERNALNAME		"NeuEncrypt"

// Get APP_VERSION
#define _TO_STRING_(x) #x
#define _TO_STRING(x) _TO_STRING_(x)
#define APP_VERSION _TO_STRING(APP_MAJOR) "." _TO_STRING(APP_MINOR) "." _TO_STRING(APP_BUILDNUMBER) "." _TO_STRING(APP_REVISION)

// Get APP_VERSION_STRING
#define APP_DOT .
#define APP_STRING NeuEncrypt, Version v
#define _PPCAT_NX(A, B, C, D) A ## B ## D ## C
#define _PPCAT(A, B, C, D) _PPCAT_NX(A, B, C, D)
#define APP_VERSION_STRING _TO_STRING(_PPCAT(APP_STRING,APP_MAJOR,APP_MINOR,APP_DOT))

// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NO_MFC                     1
#define _APS_NEXT_RESOURCE_VALUE        129
#define _APS_NEXT_COMMAND_VALUE         32771
#define _APS_NEXT_CONTROL_VALUE         1006
#define _APS_NEXT_SYMED_VALUE           110
#endif
#endif
