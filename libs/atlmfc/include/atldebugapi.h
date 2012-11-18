// This is a part of the Active Template Library.
// Copyright (C) Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.


#ifndef __ATLDEBUGAPI_H__
#define __ATLDEBUGAPI_H__

#pragma once

#ifdef __cplusplus

#pragma pack(push,_ATL_PACKING)
namespace ATL
{
extern "C" {
#endif
#define ATL_TRACE_MAX_NAME_SIZE 64

typedef enum ATLTRACESTATUS
{
	ATLTRACESTATUS_INHERIT,
	ATLTRACESTATUS_ENABLED,
	ATLTRACESTATUS_DISABLED
} ATLTRACESTATUS;

DWORD_PTR __stdcall AtlTraceOpenProcess(_In_ DWORD idProcess);
void __stdcall AtlTraceCloseProcess(_In_ DWORD_PTR dwProcess);
void __stdcall AtlTraceSnapshotProcess(_In_ DWORD_PTR dwProcess);

DWORD_PTR __stdcall AtlTraceRegister(
	_In_ HINSTANCE hInst,
	_In_opt_ int (__cdecl *fnCrtDbgReport)(
				_In_ int,
				_In_z_ const char *,
				_In_ int,
				_In_z_ const char *,
				_In_z_ const char *,...)
	);

BOOL __stdcall AtlTraceUnregister(_In_ DWORD_PTR dwModule);

DWORD_PTR __stdcall AtlTraceRegisterCategoryA(
	_In_ DWORD_PTR dwModule,
	_In_z_count_c_(ATL_TRACE_MAX_NAME_SIZE) const CHAR szCategoryName[ATL_TRACE_MAX_NAME_SIZE]);

DWORD_PTR __stdcall AtlTraceRegisterCategoryU(
	_In_ DWORD_PTR dwModule,
	_In_z_count_c_(ATL_TRACE_MAX_NAME_SIZE) const WCHAR szCategoryName[ATL_TRACE_MAX_NAME_SIZE]);

BOOL __stdcall AtlTraceModifyProcess(
	_In_ DWORD_PTR dwProcess,
	_In_ UINT nLevel,
	_In_ BOOL bEnabled,
	_In_ BOOL bFuncAndCategoryNames,
	_In_ BOOL bFileNameAndLineNo);

BOOL __stdcall AtlTraceModifyModule(
	_In_ DWORD_PTR dwProcess,
	_In_ DWORD_PTR dwModule,
	_In_ UINT nLevel,
	_In_ ATLTRACESTATUS eStatus);

BOOL __stdcall AtlTraceModifyCategory(
	_In_ DWORD_PTR dwProcess,
	_In_ DWORD_PTR dwCategory,
	_In_ UINT nLevel,
	_In_ ATLTRACESTATUS eStatus);

BOOL __stdcall AtlTraceGetProcess(
	_In_ DWORD_PTR dwProcess,
	_Out_opt_ UINT *pnLevel,
	_Out_opt_ BOOL *pbEnabled,
	_Out_opt_ BOOL *pbFuncAndCategoryNames,
	_Out_opt_ BOOL *pbFileNameAndLineNo);

BOOL __stdcall AtlTraceGetModule(
	_In_ DWORD_PTR dwProcess,
	_In_ DWORD_PTR dwModule,
	_Out_opt_ UINT *pnLevel,
	_Out_opt_ ATLTRACESTATUS *pStatus);

BOOL __stdcall AtlTraceGetCategory(
	_In_ DWORD_PTR dwProcess,
	_In_ DWORD_PTR dwCategory,
	_Out_opt_ UINT *pnLevel,
	_Out_opt_ ATLTRACESTATUS *pStatus);

_ATL_INSECURE_DEPRECATE("AtlTraceGetUpdateEventNameA is unsafe. Instead use AtlTraceGetUpdateEventNameA_s")
void __stdcall AtlTraceGetUpdateEventNameA(_Inout_z_ CHAR *pszEventName);

_ATL_INSECURE_DEPRECATE("AtlTraceGetUpdateEventNameU is unsafe. Instead use AtlTraceGetUpdateEventNameU_s")
void __stdcall AtlTraceGetUpdateEventNameU(_Inout_z_ WCHAR *pszEventName);

void __stdcall AtlTraceGetUpdateEventNameA_s(
	_Out_z_cap_(cchEventName) CHAR *pszEventName,
	_In_ size_t cchEventName);

void __stdcall AtlTraceGetUpdateEventNameU_s(
	_Out_z_cap_(cchEventName) WCHAR *pszEventName,
	_In_ size_t cchEventName);

void __cdecl AtlTraceVA(
	_In_ DWORD_PTR dwModule,
	_In_opt_z_ const char *pszFileName,
	_In_ int nLineNo,
	_In_ DWORD_PTR dwCategory,
	_In_ UINT nLevel,
	_In_z_ _Printf_format_string_ const CHAR *pszFormat, 
	_In_ va_list ptr);

void __cdecl AtlTraceVU(
	_In_ DWORD_PTR dwModule,
	_In_opt_z_ const char *pszFileName,
	_In_ int nLineNo,
	_In_ DWORD_PTR dwCategory,
	_In_ UINT nLevel,
	_In_z_ _Printf_format_string_ const WCHAR *pszFormat, 
	_In_ va_list ptr);

BOOL __stdcall AtlTraceLoadSettingsA(
	_In_opt_z_ const CHAR *pszFileName,
	_In_ DWORD_PTR dwProcess = 0);

BOOL __stdcall AtlTraceLoadSettingsU(
	_In_opt_z_ const WCHAR *pszFileName,
	_In_ DWORD_PTR dwProcess = 0);

BOOL __stdcall AtlTraceSaveSettingsA(
	_In_opt_z_ const CHAR *pszFileName,
	_In_ DWORD_PTR dwProcess = 0);

BOOL __stdcall AtlTraceSaveSettingsU(
	_In_opt_z_ const WCHAR *pszFileName,
	_In_ DWORD_PTR dwProcess = 0);

typedef struct ATLTRACESETTINGS
{
	UINT nLevel;
	ATLTRACESTATUS eStatus;
} ATLTRACESETTINGS;

typedef struct ATLTRACEPROCESSSETTINGS
{
	UINT nLevel;
	BOOL bEnabled;
	BOOL bFuncAndCategoryNames;
	BOOL bFileNameAndLineNo;
} ATLTRACEPROCESSSETTINGS;

typedef struct ATLTRACEPROCESSINFO
{
	WCHAR szName[ATL_TRACE_MAX_NAME_SIZE];
	WCHAR szPath[MAX_PATH];
	DWORD dwId;
	ATLTRACEPROCESSSETTINGS settings;
	int nModules;
} ATLTRACEPROCESSINFO;

typedef struct ATLTRACEMODULEINFO
{
	WCHAR szName[ATL_TRACE_MAX_NAME_SIZE];
	WCHAR szPath[MAX_PATH];
	ATLTRACESETTINGS settings;
	DWORD_PTR dwModule;
	int nCategories;
} ATLTRACEMODULEINFO;

typedef struct ATLTRACECATEGORYINFO
{
	WCHAR szName[ATL_TRACE_MAX_NAME_SIZE];
	ATLTRACESETTINGS settings;
	DWORD_PTR dwCategory;
} ATLTRACECATEGORYINFO;

BOOL __stdcall AtlTraceGetProcessInfo(
	_In_ DWORD_PTR dwProcess,
	_Out_ ATLTRACEPROCESSINFO* pProcessInfo);

void __stdcall AtlTraceGetModuleInfo(
	_In_ DWORD_PTR dwProcess,
	_In_ int iModule,
	_Out_ ATLTRACEMODULEINFO* pModuleInfo);

void __stdcall AtlTraceGetCategoryInfo(
	_In_ DWORD_PTR dwProcess,
	_In_ DWORD_PTR dwModule,
	_In_ int iCategory,
	_Out_ ATLTRACECATEGORYINFO* pAtlTraceCategoryInfo);

#ifdef UNICODE
#define AtlTraceRegisterCategory AtlTraceRegisterCategoryU
#define AtlTraceGetUpdateEventName AtlTraceGetUpdateEventNameU
#define AtlTraceGetUpdateEventName_s AtlTraceGetUpdateEventNameU_s
#define AtlTrace AtlTraceU
#define AtlTraceV AtlTraceVU
#define AtlTraceLoadSettings AtlTraceLoadSettingsU
#define AtlTraceSaveSettings AtlTraceSaveSettingsU

#else
#define AtlTraceRegisterCategory AtlTraceRegisterCategoryA
#define AtlTraceGetUpdateEventName AtlTraceGetUpdateEventNameA
#define AtlTraceGetUpdateEventName_s AtlTraceGetUpdateEventNameA_s
#define AtlTrace AtlTraceA
#define AtlTraceV AtlTraceVA
#define AtlTraceLoadSettings AtlTraceLoadSettingsA
#define AtlTraceSaveSettings AtlTraceSaveSettingsA

#endif

#ifdef __cplusplus
};

};  // namespace ATL
#pragma pack(pop)
#endif

#endif  // __ATLDEBUGAPI_H__
