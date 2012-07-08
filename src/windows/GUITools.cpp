//---------------------------------------------------------------------------
#include "nbafx.h"
#define NO_WIN32_LEAN_AND_MEAN
#ifndef _MSC_VER
#include <vcl.h>
#pragma hdrstop
#endif

#include <shlobj.h>
#include <Common.h>

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>

#include "GUITools.h"
#include "GUIConfiguration.h"
#include <TextsCore.h>
#include <CoreMain.h>
#include <SessionData.h>
#include <Exceptions.h>
//---------------------------------------------------------------------------
#ifndef _MSC_VER
#pragma package(smart_init)
#endif
//---------------------------------------------------------------------------
bool __fastcall FindFile(UnicodeString & Path)
{
  bool Result = FileExists(Path);
  if (!Result)
  {
    int Len = GetEnvironmentVariable(L"PATH", NULL, 0);
    if (Len > 0)
    {
      UnicodeString Paths;
      Paths.SetLength(Len - 1);
      GetEnvironmentVariable(L"PATH", reinterpret_cast<LPWSTR>(const_cast<wchar_t *>(Paths.c_str())), static_cast<DWORD>(Len));

      UnicodeString NewPath = FileSearch(ExtractFileName(Path, true), Paths);
      Result = !NewPath.IsEmpty();
      if (Result)
      {
        Path = NewPath;
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall FileExistsEx(UnicodeString Path)
{
  DEBUG_PRINTF(L"Path = %s", Path.c_str());
  return FindFile(Path);
}
//---------------------------------------------------------------------------
void __fastcall OpenSessionInPutty(const UnicodeString PuttyPath,
  TSessionData * SessionData, UnicodeString Password)
{
  UnicodeString Program, Params, Dir;
  SplitCommand(PuttyPath, Program, Params, Dir);
  Program = ExpandEnvironmentVariables(Program);
  if (FindFile(Program))
  {
    UnicodeString SessionName;
    TRegistryStorage * Storage = NULL;
    TSessionData * ExportData = NULL;
    TRegistryStorage * SourceStorage = NULL;
    // try
    {
      BOOST_SCOPE_EXIT ( (&Storage) (&ExportData) (&SourceStorage) )
      {
        delete Storage;
        delete ExportData;
        delete SourceStorage;
      } BOOST_SCOPE_EXIT_END
      Storage = new TRegistryStorage(Configuration->GetPuttySessionsKey());
      Storage->SetAccessMode(smReadWrite);
      // make it compatible with putty
      Storage->SetMungeStringValues(false);
      Storage->SetForceAnsi(true);
      if (Storage->OpenRootKey(true))
      {
        if (Storage->KeyExists(SessionData->GetStorageKey()))
        {
          SessionName = SessionData->GetSessionName();
        }
        else
        {
          SourceStorage = new TRegistryStorage(Configuration->GetPuttySessionsKey());
          SourceStorage->SetMungeStringValues(false);
          SourceStorage->SetForceAnsi(true);
          if (SourceStorage->OpenSubKey(StoredSessions->GetDefaultSettings()->GetName(), false) &&
              Storage->OpenSubKey(GUIConfiguration->GetPuttySession(), true))
          {
            Storage->Copy(SourceStorage);
            Storage->CloseSubKey();
          }

          ExportData = new TSessionData(L"");
          ExportData->Assign(SessionData);
          ExportData->SetModified(true);
          ExportData->SetName(GUIConfiguration->GetPuttySession());
          ExportData->SetPassword(L"");

          if (SessionData->GetFSProtocol() == fsFTP)
          {
            if (GUIConfiguration->GetTelnetForFtpInPutty())
            {
              ExportData->SetProtocol(ptTelnet);
              ExportData->SetPortNumber(23);
              // PuTTY  does not allow -pw for telnet
              Password = L"";
            }
            else
            {
              ExportData->SetProtocol(ptSSH);
              ExportData->SetPortNumber(22);
            }
          }

          ExportData->Save(Storage, true);
          SessionName = GUIConfiguration->GetPuttySession();
        }
      }
    }
#ifndef _MSC_VER
    __finally
    {
      delete Storage;
      delete ExportData;
      delete SourceStorage;
    }
#endif

    if (!Params.IsEmpty())
    {
      Params += L" ";
    }
    if (!Password.IsEmpty())
    {
      Params += FORMAT(L"-pw %s ", EscapePuttyCommandParam(Password).c_str());
    }
    Params += FORMAT(L"-load %s", EscapePuttyCommandParam(SessionName).c_str());

    if (!ExecuteShell(Program, Params))
    {
      throw Exception(FMTLOAD(EXECUTE_APP_ERROR, Program.c_str()));
    }
  }
  else
  {
    throw Exception(FMTLOAD(FILE_NOT_FOUND, Program.c_str()));
  }
}
//---------------------------------------------------------------------------
bool __fastcall ExecuteShell(const UnicodeString Path, const UnicodeString Params)
{
  return ((int)::ShellExecute(NULL, L"open", const_cast<wchar_t*>(Path.data()),
    const_cast<wchar_t*>(Params.data()), NULL, SW_SHOWNORMAL) > 32);
}
//---------------------------------------------------------------------------
bool __fastcall ExecuteShell(const UnicodeString Path, const UnicodeString Params,
  HANDLE & Handle)
{
  bool Result = false;

  _SHELLEXECUTEINFOW ExecuteInfo;
  memset(&ExecuteInfo, 0, sizeof(ExecuteInfo));
  ExecuteInfo.cbSize = sizeof(ExecuteInfo);
  ExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  ExecuteInfo.hwnd = reinterpret_cast<HWND>(::GetModuleHandle(0));
  ExecuteInfo.lpFile = const_cast<wchar_t *>(Path.data());
  ExecuteInfo.lpParameters = const_cast<wchar_t *>(Params.data());
  ExecuteInfo.nShow = SW_SHOW;

  Result = (::ShellExecuteEx(&ExecuteInfo) != 0);
  if (Result)
  {
    Handle = ExecuteInfo.hProcess;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall ExecuteShellAndWait(HINSTANCE Handle, const UnicodeString Path,
  const UnicodeString Params, TProcessMessagesEvent ProcessMessages)
{
  bool Result = false;
  _SHELLEXECUTEINFOW ExecuteInfo;
  memset(&ExecuteInfo, 0, sizeof(ExecuteInfo));
  ExecuteInfo.cbSize = sizeof(ExecuteInfo);
  ExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  ExecuteInfo.hwnd = reinterpret_cast<HWND>(::GetModuleHandle(0));
  ExecuteInfo.lpFile = const_cast<wchar_t *>(Path.data());
  ExecuteInfo.lpParameters = const_cast<wchar_t *>(Params.data());
  ExecuteInfo.nShow = SW_SHOW;

  Result = (ShellExecuteEx(&ExecuteInfo) != 0);
  if (Result)
  {
    if (!ProcessMessages.empty())
    {
      unsigned long WaitResult;
      do
      {
        WaitResult = WaitForSingleObject(ExecuteInfo.hProcess, 200);
        if (WaitResult == WAIT_FAILED)
        {
          throw Exception(LoadStr(DOCUMENT_WAIT_ERROR));
        }
        ProcessMessages();
      }
      while (WaitResult == WAIT_TIMEOUT);
    }
    else
    {
      WaitForSingleObject(ExecuteInfo.hProcess, INFINITE);
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall ExecuteShellAndWait(HINSTANCE Handle, const UnicodeString Command,
  TProcessMessagesEvent ProcessMessages)
{
  UnicodeString Program, Params, Dir;
  SplitCommand(Command, Program, Params, Dir);
  return ExecuteShellAndWait(Handle, Program, Params, ProcessMessages);
}
//---------------------------------------------------------------------------
bool __fastcall SpecialFolderLocation(int PathID, UnicodeString & Path)
{
  LPITEMIDLIST Pidl;
  wchar_t Buf[256];
  if (SHGetSpecialFolderLocation(NULL, PathID, &Pidl) == NO_ERROR &&
      SHGetPathFromIDList(Pidl, Buf))
  {
    Path = UnicodeString(Buf);
    return true;
  }
  return false;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall ItemsFormatString(const UnicodeString SingleItemFormat,
  const UnicodeString MultiItemsFormat, int Count, const UnicodeString FirstItem)
{
  UnicodeString Result;
  if (Count == 1)
  {
    Result = FORMAT(SingleItemFormat.c_str(), FirstItem.c_str());
  }
  else
  {
    Result = FORMAT(MultiItemsFormat.c_str(), Count);
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall ItemsFormatString(const UnicodeString SingleItemFormat,
  const UnicodeString MultiItemsFormat, TStrings * Items)
{
  return ItemsFormatString(SingleItemFormat, MultiItemsFormat,
    Items->GetCount(), (Items->GetCount() > 0 ? Items->GetStrings(0) : UnicodeString()));
}
//---------------------------------------------------------------------------
UnicodeString __fastcall FileNameFormatString(const UnicodeString SingleFileFormat,
  const UnicodeString MultiFilesFormat, TStrings * Files, bool Remote)
{
  assert(Files != NULL);
  UnicodeString Item;
  if (Files->GetCount() > 0)
  {
    Item = Remote ? UnixExtractFileName(Files->GetStrings(0)) :
      ExtractFileName(Files->GetStrings(0), true);
  }
  return ItemsFormatString(SingleFileFormat, MultiFilesFormat,
    Files->GetCount(), Item);
}
//---------------------------------------------------------------------------
UnicodeString __fastcall UniqTempDir(const UnicodeString BaseDir, const UnicodeString Identity,
  bool Mask)
{
  UnicodeString TempDir;
  do
  {
    TempDir = BaseDir.IsEmpty() ? SystemTemporaryDirectory() : BaseDir;
    TempDir = IncludeTrailingBackslash(TempDir) + Identity;
    if (Mask)
    {
      TempDir += L"?????";
    }
    else
    {
      // TempDir += IncludeTrailingBackslash(FormatDateTime(L"nnzzz", Now()));
      TDateTime dt = Now();
      unsigned short H, M, S, MS;
      dt.DecodeTime(H, M, S, MS);
      TempDir += IncludeTrailingBackslash(FORMAT(L"%02d%03d", M, MS));
    };
  }
  while (!Mask && DirectoryExists(TempDir));

  return TempDir;
}
//---------------------------------------------------------------------------
bool __fastcall DeleteDirectory(const UnicodeString DirName)
{
  TSearchRec sr = {0};
  bool retval = true;
  if (FindFirst(DirName + L"\\*", faAnyFile, sr) == 0) // VCL Function
  {
    if (FLAGSET(sr.Attr, faDirectory))
    {
      if ((sr.Name != THISDIRECTORY) && (sr.Name != PARENTDIRECTORY))
        retval = DeleteDirectory(DirName + L"\\" + sr.Name);
    }
    else
    {
      retval = DeleteFile(DirName + L"\\" + sr.Name);
    }

    if (retval)
    {
      while (FindNext(sr) == 0)
      { // VCL Function
        if (FLAGSET(sr.Attr, faDirectory))
        {
          if ((sr.Name != THISDIRECTORY) && (sr.Name != PARENTDIRECTORY))
            retval = DeleteDirectory(DirName + L"\\" + sr.Name);
        }
        else
        {
          retval = DeleteFile(DirName + L"\\" + sr.Name);
        }

        if (!retval) { break; }
      }
    }
  }
  FindClose(sr);
  if (retval) { retval = RemoveDir(DirName); } // VCL function
  return retval;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall FormatDateTimeSpan(const UnicodeString TimeFormat, TDateTime DateTime)
{
  UnicodeString Result;
  if (static_cast<int>(DateTime) > 0)
  {
    Result = IntToStr(static_cast<int>(DateTime)) + L", ";
  }
  // days are decremented, because when there are to many of them,
  // "integer overflow" error occurs
  // Result += FormatDateTime(TimeFormat, System::TDateTime(DateTime - int(DateTime)));
  TDateTime dt(DateTime - static_cast<int>(DateTime));
  unsigned short H, M, S, MS;
  dt.DecodeTime(H, M, S, MS);
  Result += FORMAT(L"%02d:%02d:%02d", H, M, S);
  return Result;
}
//---------------------------------------------------------------------------
TLocalCustomCommand::TLocalCustomCommand()
{
}
//---------------------------------------------------------------------------
TLocalCustomCommand::TLocalCustomCommand(const TCustomCommandData & Data,
    const UnicodeString & Path) :
  TFileCustomCommand(Data, Path)
{
}
//---------------------------------------------------------------------------
TLocalCustomCommand::TLocalCustomCommand(const TCustomCommandData & Data,
  const UnicodeString & Path, const UnicodeString & FileName,
  const UnicodeString & LocalFileName, const UnicodeString & FileList) :
  TFileCustomCommand(Data, Path, FileName, FileList)
{
  FLocalFileName = LocalFileName;
}
//---------------------------------------------------------------------------
int __fastcall TLocalCustomCommand::PatternLen(int Index, wchar_t PatternCmd)
{
  int Len = 0;
  if (PatternCmd == L'^')
  {
    Len = 3;
  }
  else
  {
    Len = TFileCustomCommand::PatternLen(Index, PatternCmd);
  }
  return Len;
}
//---------------------------------------------------------------------------
bool __fastcall TLocalCustomCommand::PatternReplacement(int Index,
  const UnicodeString & Pattern, UnicodeString & Replacement, bool & Delimit)
{
  bool Result = false;
  if (Pattern == L"!^!")
  {
    Replacement = FLocalFileName;
    Result = true;
  }
  else
  {
    Result = TFileCustomCommand::PatternReplacement(Index, Pattern, Replacement, Delimit);
  }
  return Result;
}
//---------------------------------------------------------------------------
void __fastcall TLocalCustomCommand::DelimitReplacement(
  UnicodeString & /*Replacement*/, wchar_t /*Quote*/)
{
  // never delimit local commands
}
//---------------------------------------------------------------------------
bool __fastcall TLocalCustomCommand::HasLocalFileName(const UnicodeString & Command)
{
  return FindPattern(Command, L'^');
}
//---------------------------------------------------------------------------
bool __fastcall TLocalCustomCommand::IsFileCommand(const UnicodeString & Command)
{
  return TFileCustomCommand::IsFileCommand(Command) || HasLocalFileName(Command);
}
//---------------------------------------------------------------------
UnicodeString __fastcall FormatBytes(__int64 Bytes, bool UseOrders)
{
  UnicodeString Result;

  if (!UseOrders || (Bytes < static_cast<__int64>(100*1024)))
  {
    // Result = FormatFloat(L"#,##0 \"B\"", Bytes);
    Result = FORMAT(L"%.0f B", static_cast<double>(Bytes));
  }
  else if (Bytes < static_cast<__int64>(100*1024*1024))
  {
    // Result = FormatFloat(L"#,##0 \"KiB\"", Bytes / 1024);
    Result = FORMAT(L"%.0f KiB", static_cast<double>(Bytes / 1024.0));
  }
  else
  {
    // Result = FormatFloat(L"#,##0 \"MiB\"", Bytes / (1024*1024));
    Result = FORMAT(L"%.0f MiB", static_cast<double>(Bytes / (1024*1024.0)));
  }
  return Result;
}
//---------------------------------------------------------------------
