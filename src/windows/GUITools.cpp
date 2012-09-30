//---------------------------------------------------------------------------
#define NO_WIN32_LEAN_AND_MEAN
#include <vcl.h>
#pragma hdrstop

#include <shlobj.h>
#include <Common.h>

#include "GUITools.h"
#include "GUIConfiguration.h"
#include <TextsCore.h>
#include <CoreMain.h>
#include <SessionData.h>
//---------------------------------------------------------------------------
#pragma package(smart_init)
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
  return FindFile(Path);
}
//---------------------------------------------------------------------------
void __fastcall OpenSessionInPutty(const UnicodeString PuttyPath,
  TSessionData * SessionData, UnicodeString Password)
{
  CALLSTACK;
  UnicodeString Program, Params, Dir;
  SplitCommand(PuttyPath, Program, Params, Dir);
  Program = ExpandEnvironmentVariables(Program);
  if (FindFile(Program))
  {
    TRACE("1");
    UnicodeString SessionName;
    TRegistryStorage * Storage = NULL;
    TSessionData * ExportData = NULL;
    TRegistryStorage * SourceStorage = NULL;
    TRY_FINALLY3 (Storage, ExportData, SourceStorage,
    {
      TRACEFMT("1a [%s]", (Configuration->GetPuttySessionsKey()));
      Storage = new TRegistryStorage(Configuration->GetPuttySessionsKey());
      Storage->SetAccessMode(smReadWrite);
      // make it compatible with putty
      Storage->SetMungeStringValues(false);
      Storage->SetForceAnsi(true);
      if (Storage->OpenRootKey(true))
      {
        TRACEFMT("2 [%s]", (SessionData->GetStorageKey()));
        if (Storage->KeyExists(SessionData->GetStorageKey()))
        {
          SessionName = SessionData->GetSessionName();
          TRACEFMT("3 [%s]", (SessionName));
        }
        else
        {
          TRACE("4");
          SourceStorage = new TRegistryStorage(Configuration->GetPuttySessionsKey());
          SourceStorage->SetMungeStringValues(false);
          SourceStorage->SetForceAnsi(true);
          if (SourceStorage->OpenSubKey(StoredSessions->GetDefaultSettings()->GetName(), false) &&
              Storage->OpenSubKey(GUIConfiguration->GetPuttySession(), true))
          {
            TRACE("5");
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
            TRACE("6");
            if (GUIConfiguration->GetTelnetForFtpInPutty())
            {
              TRACE("7");
              ExportData->SetProtocol(ptTelnet);
              ExportData->SetPortNumber(23);
              // PuTTY  does not allow -pw for telnet
              Password = L"";
            }
            else
            {
              TRACE("8");
              ExportData->SetProtocol(ptSSH);
              ExportData->SetPortNumber(22);
            }
          }

          ExportData->Save(Storage, true);
          SessionName = GUIConfiguration->GetPuttySession();
        }
      }
    }
    ,
    {
      TRACE("9");
      delete Storage;
      delete ExportData;
      delete SourceStorage;
    }
    );

    if (!Params.IsEmpty())
    {
      TRACE("10");
      Params += L" ";
    }
    if (!Password.IsEmpty())
    {
      TRACE("11");
      Params += FORMAT(L"-pw %s ", EscapePuttyCommandParam(Password).c_str());
    }
    Params += FORMAT(L"-load %s", EscapePuttyCommandParam(SessionName).c_str());

    TRACEFMT("11a [%s] [%s]", (Program, Params));
    if (!ExecuteShell(Program, Params))
    {
      TRACE("12");
      throw Exception(FMTLOAD(EXECUTE_APP_ERROR, Program.c_str()));
    }
  }
  else
  {
    TRACE("13");
    throw Exception(FMTLOAD(FILE_NOT_FOUND, Program.c_str()));
  }
}
//---------------------------------------------------------------------------
bool __fastcall ExecuteShell(const UnicodeString Path, const UnicodeString Params)
{
  CALLSTACK;
  TRACEFMT("1 [%s] [%s]", (Path, Params));
  return ((int)::ShellExecute(NULL, L"open", const_cast<wchar_t*>(Path.data()),
    const_cast<wchar_t*>(Params.data()), NULL, SW_SHOWNORMAL) > 32);
}
//---------------------------------------------------------------------------
bool __fastcall ExecuteShell(const UnicodeString Path, const UnicodeString Params,
  HANDLE & Handle)
{
  CALLSTACK;
  bool Result = false;

  TRACE("1");
  TShellExecuteInfoW ExecuteInfo;
  memset(&ExecuteInfo, 0, sizeof(ExecuteInfo));
  ExecuteInfo.cbSize = sizeof(ExecuteInfo);
  ExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  ExecuteInfo.hwnd = reinterpret_cast<HWND>(::GetModuleHandle(0));
  ExecuteInfo.lpFile = const_cast<wchar_t *>(Path.data());
  ExecuteInfo.lpParameters = const_cast<wchar_t *>(Params.data());
  ExecuteInfo.nShow = SW_SHOW;

  TRACE("2");
  Result = (::ShellExecuteEx(&ExecuteInfo) != 0);
  if (Result)
  {
    TRACE("3");
    Handle = ExecuteInfo.hProcess;
  }
  TRACE("/");
  return Result;
}
//---------------------------------------------------------------------------
bool __fastcall ExecuteShellAndWait(HINSTANCE Handle, const UnicodeString Path,
  const UnicodeString Params, TProcessMessagesEvent ProcessMessages)
{
  bool Result = false;

  TShellExecuteInfoW ExecuteInfo;
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
    if (ProcessMessages != NULL)
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
  wchar_t Buf[MAX_PATH];
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
    Items->Count, (Items->Count > 0 ? Items->Strings[0] : UnicodeString()));
}
//---------------------------------------------------------------------------
UnicodeString __fastcall FileNameFormatString(const UnicodeString SingleFileFormat,
  const UnicodeString MultiFilesFormat, TStrings * Files, bool Remote)
{
  assert(Files != NULL);
  UnicodeString Item;
  if (Files->Count > 0)
  {
    Item = Remote ? UnixExtractFileName(Files->Strings[0]) :
      ExtractFileName(Files->Strings[0], true);
  }
  return ItemsFormatString(SingleFileFormat, MultiFilesFormat,
    Files->Count, Item);
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
#ifndef _MSC_VER
      TempDir += IncludeTrailingBackslash(FormatDateTime(L"nnzzz", Now()));
#else
      TDateTime dt = Now();
      unsigned short H, M, S, MS;
      dt.DecodeTime(H, M, S, MS);
      TempDir += IncludeTrailingBackslash(FORMAT(L"%02d%03d", M, MS));
#endif
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
#ifndef _MSC_VER
  Result += FormatDateTime(TimeFormat, DateTime - int(DateTime));
#else
  TDateTime dt(DateTime - static_cast<int>(DateTime));
  unsigned short H, M, S, MS;
  dt.DecodeTime(H, M, S, MS);
  Result += FORMAT(L"%02d:%02d:%02d", H, M, S);
#endif
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
