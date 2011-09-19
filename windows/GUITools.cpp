//---------------------------------------------------------------------------
#include "stdafx.h"
#define NO_WIN32_LEAN_AND_MEAN

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
bool FindFile(std::wstring & Path)
{
  bool Result = FileExists(Path);
  if (!Result)
  {
    int Len = GetEnvironmentVariable(L"PATH", NULL, 0);
    if (Len > 0)
    {
      std::wstring Paths;
      Paths.resize(Len - 1);
      // FIXME GetEnvironmentVariable(L"PATH", Paths.c_str(), Len);

      std::wstring NewPath = FileSearch(ExtractFileName(Path, true), Paths);
      Result = !NewPath.empty();
      if (Result)
      {
        Path = NewPath;
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool FileExistsEx(std::wstring Path)
{
  return FindFile(Path);
}
//---------------------------------------------------------------------------
void OpenSessionInPutty(const std::wstring PuttyPath,
  TSessionData * SessionData, std::wstring Password)
{
  std::wstring Program, Params, Dir;
  SplitCommand(PuttyPath, Program, Params, Dir);
  Program = ExpandEnvironmentVariables(Program);
  if (FindFile(Program))
  {
    std::wstring SessionName;
    TRegistryStorage * Storage = NULL;
    TSessionData * ExportData = NULL;
    TRegistryStorage * SourceStorage = NULL;
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

    if (!Params.empty())
    {
      Params += L" ";
    }
    if (!Password.empty())
    {
      Params += FORMAT(L"-pw %s ", (EscapePuttyCommandParam(Password)));
    }
    Params += FORMAT(L"-load %s", (EscapePuttyCommandParam(SessionName)));

    if (!ExecuteShell(Program, Params))
    {
      throw ExtException(FMTLOAD(EXECUTE_APP_ERROR, Program.c_str()));
    }
  }
  else
  {
    throw ExtException(FMTLOAD(FILE_NOT_FOUND, Program.c_str()));
  }
}
//---------------------------------------------------------------------------
bool ExecuteShell(const std::wstring Path, const std::wstring Params)
{
  return false; // FIXME ((int)ShellExecute(NULL, L"open", (char*)Path.data(),
    // (char*)Params.data(), NULL, SW_SHOWNORMAL) > 32);
}
//---------------------------------------------------------------------------
bool ExecuteShell(const std::wstring Path, const std::wstring Params,
  HANDLE & Handle)
{
  bool Result = false;
/* // FIXME
  TShellExecuteInfo ExecuteInfo;
  memset(&ExecuteInfo, 0, sizeof(ExecuteInfo));
  ExecuteInfo.cbSize = sizeof(ExecuteInfo);
  ExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  ExecuteInfo.hwnd = Application->Handle;
  ExecuteInfo.lpFile = (char*)Path.data();
  ExecuteInfo.lpParameters = (char*)Params.data();
  ExecuteInfo.nShow = SW_SHOW;

  Result = (ShellExecuteEx(&ExecuteInfo) != 0);
  if (Result)
  {
    Handle = ExecuteInfo.hProcess;
  }
  */
  return Result;
}
//---------------------------------------------------------------------------
bool ExecuteShellAndWait(HWND Handle, const std::wstring Path,
  const std::wstring Params, const processmessages_signal_type &ProcessMessages)
{
  bool Result = false;
/* // FIXME 
  TShellExecuteInfo ExecuteInfo;
  memset(&ExecuteInfo, 0, sizeof(ExecuteInfo));
  ExecuteInfo.cbSize = sizeof(ExecuteInfo);
  ExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  ExecuteInfo.hwnd = Handle;
  ExecuteInfo.lpFile = (char*)Path.data();
  ExecuteInfo.lpParameters = (char*)Params.data();
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
          throw std::exception(LoadStr(DOCUMENT_WAIT_ERROR));
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
  */
  return Result;
}
//---------------------------------------------------------------------------
bool ExecuteShellAndWait(HWND Handle, const std::wstring Command,
  const processmessages_signal_type &ProcessMessages)
{
  std::wstring Program, Params, Dir;
  SplitCommand(Command, Program, Params, Dir);
  return ExecuteShellAndWait(Handle, Program, Params, ProcessMessages);
}
//---------------------------------------------------------------------------
bool SpecialFolderLocation(int PathID, std::wstring & Path)
{
  LPITEMIDLIST Pidl;
  char Buf[256];
  /* // FIXME 
  if (SHGetSpecialFolderLocation(NULL, PathID, &Pidl) == NO_ERROR &&
      SHGetPathFromIDList(Pidl, Buf))
  {
    Path = std::wstring(Buf);
    return true;
  }
  */
  return false;
}
//---------------------------------------------------------------------------
std::wstring ItemsFormatString(const std::wstring SingleItemFormat,
  const std::wstring MultiItemsFormat, int Count, const std::wstring FirstItem)
{
  std::wstring Result;
  if (Count == 1)
  {
    Result = FORMAT(SingleItemFormat.c_str(), FirstItem);
  }
  else
  {
    Result = FORMAT(MultiItemsFormat.c_str(), Count);
  }
  return Result;
}
//---------------------------------------------------------------------------
std::wstring ItemsFormatString(const std::wstring SingleItemFormat,
  const std::wstring MultiItemsFormat, TStrings * Items)
{
  return ItemsFormatString(SingleItemFormat, MultiItemsFormat,
    Items->GetCount(), (Items->GetCount() > 0 ? Items->GetString(0) : std::wstring()));
}
//---------------------------------------------------------------------------
std::wstring FileNameFormatString(const std::wstring SingleFileFormat,
  const std::wstring MultiFilesFormat, TStrings * Files, bool Remote)
{
  assert(Files != NULL);
  std::wstring Item;
  if (Files->GetCount() > 0)
  {
    Item = Remote ? UnixExtractFileName(Files->GetString(0)) :
      ExtractFileName(Files->GetString(0), true);
  }
  return ItemsFormatString(SingleFileFormat, MultiFilesFormat,
    Files->GetCount(), Item);
}
//---------------------------------------------------------------------
std::wstring FormatBytes(__int64 Bytes, bool UseOrders)
{
  std::wstring Result;

  if (!UseOrders || (Bytes < __int64(100*1024)))
  {
    Result = FormatFloat(L"#,##0 \"B\"", Bytes);
  }
  else if (Bytes < __int64(100*1024*1024))
  {
    Result = FormatFloat(L"#,##0 \"KiB\"", Bytes / 1024);
  }
  else
  {
    Result = FormatFloat(L"#,##0 \"MiB\"", Bytes / (1024*1024));
  }
  return Result;
}
//---------------------------------------------------------------------------
std::wstring UniqTempDir(const std::wstring BaseDir, const std::wstring Identity,
  bool Mask)
{
  std::wstring TempDir;
  do
  {
    TempDir = BaseDir.empty() ? SystemTemporaryDirectory() : BaseDir;
    TempDir = IncludeTrailingBackslash(TempDir) + Identity;
    if (Mask)
    {
      TempDir += L"?????";
    }
    else
    {
      TempDir += IncludeTrailingBackslash(FormatDateTime(L"nnzzz", Now()));
    };
  }
  while (!Mask && DirectoryExists(TempDir));

  return TempDir;
}
//---------------------------------------------------------------------------
bool DeleteDirectory(const std::wstring DirName)
{
  bool retval = true;
  /* // FIXME 
  TSearchRec sr;
  if (FindFirst(DirName + L"\\*", faAnyFile, sr) == 0) // VCL Function
  {
    if (FLAGSET(sr.Attr, faDirectory))
    {
      if (sr.Name != L"." && sr.Name != L"..")
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
          if (sr.Name != "." && sr.Name != L"..")
            retval = DeleteDirectory(DirName + L"\\" + sr.Name);
        }
        else
        {
          retval = DeleteFile(DirName + L"\\" + sr.Name);
        }

        if (!retval) break;
      }
    }
  }
  FindClose(sr);
  if (retval) retval = RemoveDir(DirName); // VCL function
  */
  return retval;
}
//---------------------------------------------------------------------------
std::wstring FormatDateTimeSpan(const std::wstring TimeFormat, TDateTime DateTime)
{
  std::wstring Result;
  if (int(DateTime) > 0)
  {
    Result = IntToStr(int(DateTime)) + L", ";
  }
  // days are decremented, because when there are to many of them,
  // "integer overflow" error occurs
  Result += FormatDateTime(TimeFormat, TDateTime(DateTime - int(DateTime)));
  return Result;
}
//---------------------------------------------------------------------------
TLocalCustomCommand::TLocalCustomCommand()
{
}
//---------------------------------------------------------------------------
TLocalCustomCommand::TLocalCustomCommand(const TCustomCommandData & Data,
    const std::wstring & Path) :
  TFileCustomCommand(Data, Path)
{
}
//---------------------------------------------------------------------------
TLocalCustomCommand::TLocalCustomCommand(const TCustomCommandData & Data,
  const std::wstring & Path, const std::wstring & FileName,
  const std::wstring & LocalFileName, const std::wstring & FileList) :
  TFileCustomCommand(Data, Path, FileName, FileList)
{
  FLocalFileName = LocalFileName;
}
//---------------------------------------------------------------------------
int TLocalCustomCommand::PatternLen(int Index, char PatternCmd)
{
  int Len;
  if (PatternCmd == '^')
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
bool TLocalCustomCommand::PatternReplacement(int Index,
  const std::wstring & Pattern, std::wstring & Replacement, bool & Delimit)
{
  bool Result;
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
void TLocalCustomCommand::DelimitReplacement(
  std::wstring & /*Replacement*/, char /*Quote*/)
{
  // never delimit local commands
}
//---------------------------------------------------------------------------
bool TLocalCustomCommand::HasLocalFileName(const std::wstring & Command)
{
  return FindPattern(Command, '^');
}
//---------------------------------------------------------------------------
bool TLocalCustomCommand::IsFileCommand(const std::wstring & Command)
{
  return TFileCustomCommand::IsFileCommand(Command) || HasLocalFileName(Command);
}
