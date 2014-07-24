//---------------------------------------------------------------------------
#include <vcl.h>
#pragma hdrstop

#include <shlobj.h>
#include <Common.h>

#include "GUITools.h"
#include "GUIConfiguration.h"
#include <TextsCore.h>
#include <CoreMain.h>
#include <SessionData.h>
#include <Interface.h>
//---------------------------------------------------------------------------
extern const UnicodeString PageantTool = L"pageant.exe";
extern const UnicodeString PuttygenTool = L"puttygen.exe";
//---------------------------------------------------------------------------
template<class TEditControl>
void ValidateMaskEditT(const UnicodeString & Mask, TEditControl * Edit, int ForceDirectoryMasks)
{
  assert(Edit != nullptr);
  TFileMasks Masks(ForceDirectoryMasks);
  try
  {
    Masks = Mask;
  }
  catch (EFileMasksException & E)
  {
    ShowExtendedException(&E);
    Edit->SetFocus();
    // This does not work for TEdit and TMemo (descendants of TCustomEdit) anymore,
    // as it re-selects whole text on exception in TCustomEdit.CMExit
//    Edit->SelStart = E.ErrorStart - 1;
//    Edit->SelLength = E.ErrorLen;
    Abort();
  }
}
//---------------------------------------------------------------------------
void ValidateMaskEdit(TFarComboBox * Edit)
{
  ValidateMaskEditT(Edit->GetText(), Edit, -1);
}
//---------------------------------------------------------------------------
void ValidateMaskEdit(TFarEdit * Edit)
{
  ValidateMaskEditT(Edit->GetText(), Edit, -1);
}

//---------------------------------------------------------------------------
bool FindFile(UnicodeString & Path)
{
  bool Result = ::FileExists(Path);
  if (!Result)
  {
    intptr_t Len = GetEnvironmentVariable(L"PATH", nullptr, 0);
    if (Len > 0)
    {
      UnicodeString Paths;
      Paths.SetLength(Len - 1);
      GetEnvironmentVariable(L"PATH", reinterpret_cast<LPWSTR>(const_cast<wchar_t *>(Paths.c_str())), static_cast<DWORD>(Len));

      UnicodeString NewPath = FileSearch(::ExtractFileName(Path, true), Paths);
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
bool FileExistsEx(const UnicodeString & Path)
{
  UnicodeString LocalPath = Path;
  return FindFile(LocalPath);
}
//---------------------------------------------------------------------------
void OpenSessionInPutty(const UnicodeString & PuttyPath,
  TSessionData * SessionData, const UnicodeString & UserName, const UnicodeString & Password)
{
  UnicodeString Program, Params, Dir;
  SplitCommand(PuttyPath, Program, Params, Dir);
  Program = ExpandEnvironmentVariables(Program);
  if (FindFile(Program))
  {
    Params = ExpandEnvironmentVariables(Params);
    UnicodeString Password = GetGUIConfiguration()->GetPuttyPassword() ? SessionData->GetPassword() : UnicodeString();
    UnicodeString Psw = Password;
    UnicodeString SessionName;
    std::unique_ptr<TRegistryStorage> Storage(new TRegistryStorage(GetConfiguration()->GetPuttySessionsKey()));
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
        std::unique_ptr<TRegistryStorage> SourceStorage(new TRegistryStorage(GetConfiguration()->GetPuttySessionsKey()));
        SourceStorage->SetMungeStringValues(false);
        SourceStorage->SetForceAnsi(true);
        if (SourceStorage->OpenSubKey(StoredSessions->GetDefaultSettings()->GetName(), false) &&
            Storage->OpenSubKey(GetGUIConfiguration()->GetPuttySession(), true))
        {
          Storage->Copy(SourceStorage.get());
          Storage->CloseSubKey();
        }

        std::unique_ptr<TSessionData> ExportData(new TSessionData(L""));
        ExportData->Assign(SessionData);
        ExportData->SetModified(true);
        ExportData->SetName(GetGUIConfiguration()->GetPuttySession());
        ExportData->SetPassword(L"");

        if (SessionData->GetFSProtocol() == fsFTP)
        {
          if (GetGUIConfiguration()->GetTelnetForFtpInPutty())
          {
            ExportData->SetPuttyProtocol(PuttyTelnetProtocol);
            ExportData->SetPortNumber(TelnetPortNumber);
            // PuTTY  does not allow -pw for telnet
            Psw = L"";
          }
          else
          {
            ExportData->SetPuttyProtocol(PuttySshProtocol);
            ExportData->SetPortNumber(SshPortNumber);
          }
        }

        ExportData->Save(Storage.get(), true);
        SessionName = GetGUIConfiguration()->GetPuttySession();
      }
    }

    if (!Params.IsEmpty())
    {
      Params += L" ";
    }
    if (!Psw.IsEmpty())
    {
      Params += FORMAT(L"-pw %s ", EscapePuttyCommandParam(Psw).c_str());
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
bool FindTool(const UnicodeString & Name, UnicodeString & Path)
{
  UnicodeString AppPath = ::IncludeTrailingBackslash(::ExtractFilePath(GetConfiguration()->ModuleFileName()));
  Path = AppPath + Name;
  bool Result = true;
  if (!::FileExists(Path))
  {
    Path = AppPath + L"PuTTY\\" + Name;
    if (!::FileExists(Path))
    {
      Path = Name;
      if (!FindFile(Path))
      {
        Result = false;
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
bool ExecuteShell(const UnicodeString & Path, const UnicodeString & Params)
{
  return ((intptr_t)::ShellExecute(nullptr, L"open", const_cast<wchar_t *>(Path.data()),
    const_cast<wchar_t *>(Params.data()), nullptr, SW_SHOWNORMAL) > 32);
}
//---------------------------------------------------------------------------
bool ExecuteShell(const UnicodeString & Path, const UnicodeString & Params,
  HANDLE & Handle)
{
  TShellExecuteInfoW ExecuteInfo;
  memset(&ExecuteInfo, 0, sizeof(ExecuteInfo));
  ExecuteInfo.cbSize = sizeof(ExecuteInfo);
  ExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  ExecuteInfo.hwnd = reinterpret_cast<HWND>(::GetModuleHandle(0));
  ExecuteInfo.lpFile = const_cast<wchar_t *>(Path.data());
  ExecuteInfo.lpParameters = const_cast<wchar_t *>(Params.data());
  ExecuteInfo.nShow = SW_SHOW;

  bool Result = (::ShellExecuteEx(&ExecuteInfo) != 0);
  if (Result)
  {
    Handle = ExecuteInfo.hProcess;
  }
  return Result;
}
//---------------------------------------------------------------------------
bool ExecuteShellAndWait(HINSTANCE Handle, const UnicodeString & Path,
  const UnicodeString & Params, TProcessMessagesEvent ProcessMessages)
{
  TShellExecuteInfoW ExecuteInfo;
  memset(&ExecuteInfo, 0, sizeof(ExecuteInfo));
  ExecuteInfo.cbSize = sizeof(ExecuteInfo);
  ExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  ExecuteInfo.hwnd = reinterpret_cast<HWND>(::GetModuleHandle(0));
  ExecuteInfo.lpFile = const_cast<wchar_t *>(Path.data());
  ExecuteInfo.lpParameters = const_cast<wchar_t *>(Params.data());
  ExecuteInfo.nShow = SW_SHOW;

  bool Result = (ShellExecuteEx(&ExecuteInfo) != 0);
  if (Result)
  {
    if (ProcessMessages != nullptr)
    {
      uint32_t WaitResult;
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
bool ExecuteShellAndWait(HINSTANCE Handle, const UnicodeString & Command,
  TProcessMessagesEvent ProcessMessages)
{
  UnicodeString Program, Params, Dir;
  SplitCommand(Command, Program, Params, Dir);
  return ExecuteShellAndWait(Handle, Program, Params, ProcessMessages);
}
//---------------------------------------------------------------------------
bool SpecialFolderLocation(int PathID, UnicodeString & Path)
{
  LPITEMIDLIST Pidl;
  wchar_t Buf[MAX_PATH];
  if (SHGetSpecialFolderLocation(nullptr, PathID, &Pidl) == NO_ERROR &&
      SHGetPathFromIDList(Pidl, Buf))
  {
    Path = UnicodeString(Buf);
    return true;
  }
  return false;
}
//---------------------------------------------------------------------------
UnicodeString GetPersonalFolder()
{
  UnicodeString Result;
  SpecialFolderLocation(CSIDL_PERSONAL, Result);

  if (IsWine())
  {
    UnicodeString WineHostHome;
    int Len = GetEnvironmentVariable(L"WINE_HOST_HOME", NULL, 0);
    if (Len > 0)
    {
      WineHostHome.SetLength(Len - 1);
      GetEnvironmentVariable(L"WINE_HOST_HOME", (LPWSTR)WineHostHome.c_str(), Len);
    }
    if (!WineHostHome.IsEmpty())
    {
      UnicodeString WineHome = L"Z:" + ToUnixPath(WineHostHome);
      if (DirectoryExists(WineHome))
      {
        Result = WineHome;
      }
    }
    else
    {
      // Should we use WinAPI GetUserName() instead?
      UnicodeString UserName;
      int Len = GetEnvironmentVariable(L"USERNAME", NULL, 0);
      if (Len > 0)
      {
        UserName.SetLength(Len - 1);
        GetEnvironmentVariable(L"USERNAME", (LPWSTR)UserName.c_str(), Len);
      }
      if (!UserName.IsEmpty())
      {
        UnicodeString WineHome = L"Z:\\home\\" + UserName;
        if (DirectoryExists(WineHome))
        {
          Result = WineHome;
        }
      }
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
UnicodeString ItemsFormatString(const UnicodeString & SingleItemFormat,
  const UnicodeString & MultiItemsFormat, intptr_t Count, const UnicodeString & FirstItem)
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
UnicodeString ItemsFormatString(const UnicodeString & SingleItemFormat,
  const UnicodeString & MultiItemsFormat, const TStrings * Items)
{
  return ItemsFormatString(SingleItemFormat, MultiItemsFormat,
    Items->GetCount(), (Items->GetCount() > 0 ? Items->GetString(0) : UnicodeString()));
}
//---------------------------------------------------------------------------
UnicodeString FileNameFormatString(const UnicodeString & SingleFileFormat,
  const UnicodeString & MultiFilesFormat, const TStrings * Files, bool Remote)
{
  assert(Files != nullptr);
  UnicodeString Item;
  if (Files->GetCount() > 0)
  {
    Item = Remote ? ::UnixExtractFileName(Files->GetString(0)) :
      ::ExtractFileName(Files->GetString(0), true);
  }
  return ItemsFormatString(SingleFileFormat, MultiFilesFormat,
    Files->GetCount(), Item);
}
//---------------------------------------------------------------------------
UnicodeString UniqTempDir(const UnicodeString & BaseDir, const UnicodeString & Identity,
  bool Mask)
{
  UnicodeString TempDir;
  do
  {
    TempDir = BaseDir.IsEmpty() ? SystemTemporaryDirectory() : BaseDir;
    TempDir = ::IncludeTrailingBackslash(TempDir) + Identity;
    if (Mask)
    {
      TempDir += L"?????";
    }
    else
    {
#if defined(__BORLANDC__)
      TempDir += ::IncludeTrailingBackslash(FormatDateTime(L"nnzzz", Now()));
#else
      TDateTime dt = Now();
      uint16_t H, M, S, MS;
      dt.DecodeTime(H, M, S, MS);
      TempDir += ::IncludeTrailingBackslash(FORMAT(L"%02d%03d", M, MS));
#endif
    };
  }
  while (!Mask && DirectoryExists(TempDir));

  return TempDir;
}
//---------------------------------------------------------------------------
bool DeleteDirectory(const UnicodeString & DirName)
{
  TSearchRecChecked SearchRec;
  bool retval = true;
  if (FindFirst(DirName + L"\\*", faAnyFile, SearchRec) == 0) // VCL Function
  {
    if (FLAGSET(SearchRec.Attr, faDirectory))
    {
      if ((SearchRec.Name != THISDIRECTORY) && (SearchRec.Name != PARENTDIRECTORY))
        retval = DeleteDirectory(DirName + L"\\" + SearchRec.Name);
    }
    else
    {
      retval = Sysutils::DeleteFile(ApiPath(DirName + L"\\" + SearchRec.Name));
    }

    if (retval)
    {
      while (FindNextChecked(SearchRec) == 0)
      { // VCL Function
        if (FLAGSET(SearchRec.Attr, faDirectory))
        {
          if ((SearchRec.Name != THISDIRECTORY) && (SearchRec.Name != PARENTDIRECTORY))
            retval = DeleteDirectory(DirName + L"\\" + SearchRec.Name);
        }
        else
        {
          retval = Sysutils::DeleteFile(ApiPath(DirName + L"\\" + SearchRec.Name));
        }

        if (!retval)
        {
          break;
        }
      }
    }
  }
  FindClose(SearchRec);
  if (retval)
  {
    retval = RemoveDir(DirName); // VCL function
  }
  return retval;
}
//---------------------------------------------------------------------------
UnicodeString FormatDateTimeSpan(const UnicodeString & /* TimeFormat */, const TDateTime & DateTime)
{
  UnicodeString Result;
  if (static_cast<int>(DateTime) > 0)
  {
    Result = IntToStr(static_cast<intptr_t>((double)DateTime)) + L", ";
  }
  // days are decremented, because when there are to many of them,
  // "integer overflow" error occurs
#if defined(__BORLANDC__)
  Result += FormatDateTime(TimeFormat, DateTime - int(DateTime));
#else
  TDateTime dt(DateTime - static_cast<int>(DateTime));
  uint16_t H, M, S, MS;
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
  const UnicodeString & Path, const UnicodeString & AFileName,
  const UnicodeString & LocalFileName, const UnicodeString & FileList) :
  TFileCustomCommand(Data, Path, AFileName, FileList)
{
  FLocalFileName = LocalFileName;
}
//---------------------------------------------------------------------------
intptr_t TLocalCustomCommand::PatternLen(const UnicodeString & Command, intptr_t Index)
{
  intptr_t Len = 0;
  if (Command[Index + 1] == L'^')
  {
    Len = 3;
  }
  else
  {
    Len = TFileCustomCommand::PatternLen(Command, Index);
  }
  return Len;
}
//---------------------------------------------------------------------------
bool TLocalCustomCommand::PatternReplacement(const UnicodeString & Pattern,
  UnicodeString & Replacement, bool & Delimit)
{
  bool Result = false;
  if (Pattern == L"!^!")
  {
    Replacement = FLocalFileName;
    Result = true;
  }
  else
  {
    Result = TFileCustomCommand::PatternReplacement(Pattern, Replacement, Delimit);
  }
  return Result;
}
//---------------------------------------------------------------------------
void TLocalCustomCommand::DelimitReplacement(
  UnicodeString & /*Replacement*/, wchar_t /*Quote*/)
{
  // never delimit local commands
}
//---------------------------------------------------------------------------
bool TLocalCustomCommand::HasLocalFileName(const UnicodeString & Command)
{
  return FindPattern(Command, L'^');
}
//---------------------------------------------------------------------------
bool TLocalCustomCommand::IsFileCommand(const UnicodeString & Command)
{
  return TFileCustomCommand::IsFileCommand(Command) || HasLocalFileName(Command);
}
//---------------------------------------------------------------------
