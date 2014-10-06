
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

extern const UnicodeString PageantTool = L"pageant.exe";
extern const UnicodeString PuttygenTool = L"puttygen.exe";

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
    Classes::Abort();
  }
}

void ValidateMaskEdit(TFarComboBox * Edit)
{
  ValidateMaskEditT(Edit->GetText(), Edit, -1);
}

void ValidateMaskEdit(TFarEdit * Edit)
{
  ValidateMaskEditT(Edit->GetText(), Edit, -1);
}


bool FindFile(UnicodeString & APath)
{
  bool Result = Sysutils::FileExists(APath);
  if (!Result)
  {
    intptr_t Len = GetEnvironmentVariable(L"PATH", nullptr, 0);
    if (Len > 0)
    {
      UnicodeString Paths;
      Paths.SetLength(Len - 1);
      GetEnvironmentVariable(L"PATH", reinterpret_cast<LPWSTR>(const_cast<wchar_t *>(Paths.c_str())), static_cast<DWORD>(Len));

      UnicodeString NewPath = Sysutils::FileSearch(core::ExtractFileName(APath, true), Paths);
      Result = !NewPath.IsEmpty();
      if (Result)
      {
        APath = NewPath;
      }
    }
  }
  return Result;
}

bool FileExistsEx(const UnicodeString & APath)
{
  UnicodeString LocalPath = APath;
  return FindFile(LocalPath);
}

void OpenSessionInPutty(const UnicodeString & PuttyPath,
  TSessionData * SessionData)
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
    //Params += FORMAT(L"-load %s", EscapePuttyCommandParam(SessionName).c_str());
    Params += FORMAT(L"-l %s ", EscapePuttyCommandParam(SessionData->GetUserNameExpanded()).c_str());
    Params += FORMAT(L"-P %d ", SessionData->GetPortNumber());
    Params += FORMAT(L"%s ", EscapePuttyCommandParam(SessionData->GetHostNameExpanded()).c_str());

    if (!ExecuteShell(Program, Params))
    {
      throw Sysutils::Exception(FMTLOAD(EXECUTE_APP_ERROR, Program.c_str()));
    }
  }
  else
  {
    throw Sysutils::Exception(FMTLOAD(FILE_NOT_FOUND, Program.c_str()));
  }
}

bool FindTool(const UnicodeString & Name, UnicodeString & APath)
{
  UnicodeString AppPath = Sysutils::IncludeTrailingBackslash(Sysutils::ExtractFilePath(GetConfiguration()->ModuleFileName()));
  APath = AppPath + Name;
  bool Result = true;
  if (!Sysutils::FileExists(APath))
  {
    APath = AppPath + L"PuTTY\\" + Name;
    if (!Sysutils::FileExists(APath))
    {
      APath = Name;
      if (!FindFile(APath))
      {
        Result = false;
      }
    }
  }
  return Result;
}

bool ExecuteShell(const UnicodeString & APath, const UnicodeString & Params)
{
  return ((intptr_t)::ShellExecute(nullptr, L"open", const_cast<wchar_t *>(APath.data()),
    const_cast<wchar_t *>(Params.data()), nullptr, SW_SHOWNORMAL) > 32);
}

bool ExecuteShell(const UnicodeString & APath, const UnicodeString & Params,
  HANDLE & Handle)
{
  TShellExecuteInfoW ExecuteInfo;
  memset(&ExecuteInfo, 0, sizeof(ExecuteInfo));
  ExecuteInfo.cbSize = sizeof(ExecuteInfo);
  ExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  ExecuteInfo.hwnd = reinterpret_cast<HWND>(::GetModuleHandle(0));
  ExecuteInfo.lpFile = const_cast<wchar_t *>(APath.data());
  ExecuteInfo.lpParameters = const_cast<wchar_t *>(Params.data());
  ExecuteInfo.nShow = SW_SHOW;

  bool Result = (::ShellExecuteEx(&ExecuteInfo) != 0);
  if (Result)
  {
    Handle = ExecuteInfo.hProcess;
  }
  return Result;
}

bool ExecuteShellAndWait(HINSTANCE /* Handle */, const UnicodeString & APath,
  const UnicodeString & Params, TProcessMessagesEvent ProcessMessages)
{
  TShellExecuteInfoW ExecuteInfo;
  memset(&ExecuteInfo, 0, sizeof(ExecuteInfo));
  ExecuteInfo.cbSize = sizeof(ExecuteInfo);
  ExecuteInfo.fMask = SEE_MASK_NOCLOSEPROCESS;
  ExecuteInfo.hwnd = reinterpret_cast<HWND>(::GetModuleHandle(0));
  ExecuteInfo.lpFile = const_cast<wchar_t *>(APath.data());
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
        WaitResult = ::WaitForSingleObject(ExecuteInfo.hProcess, 200);
        if (WaitResult == WAIT_FAILED)
        {
          throw Sysutils::Exception(LoadStr(DOCUMENT_WAIT_ERROR));
        }
        ProcessMessages();
      }
      while (WaitResult == WAIT_TIMEOUT);
    }
    else
    {
      ::WaitForSingleObject(ExecuteInfo.hProcess, INFINITE);
    }
  }
  return Result;
}

bool ExecuteShellAndWait(HINSTANCE Handle, const UnicodeString & Command,
  TProcessMessagesEvent ProcessMessages)
{
  UnicodeString Program, Params, Dir;
  SplitCommand(Command, Program, Params, Dir);
  return ExecuteShellAndWait(Handle, Program, Params, ProcessMessages);
}

bool SpecialFolderLocation(int PathID, UnicodeString & APath)
{
  LPITEMIDLIST Pidl;
  wchar_t Buf[MAX_PATH];
  if (SHGetSpecialFolderLocation(nullptr, PathID, &Pidl) == NO_ERROR &&
      SHGetPathFromIDList(Pidl, Buf))
  {
    APath = UnicodeString(Buf);
    return true;
  }
  return false;
}

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
      UnicodeString WineHome = L"Z:" + core::ToUnixPath(WineHostHome);
      if (Sysutils::DirectoryExists(WineHome))
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
        if (Sysutils::DirectoryExists(WineHome))
        {
          Result = WineHome;
        }
      }
    }
  }
  return Result;
}

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

UnicodeString ItemsFormatString(const UnicodeString & SingleItemFormat,
  const UnicodeString & MultiItemsFormat, const Classes::TStrings * Items)
{
  return ItemsFormatString(SingleItemFormat, MultiItemsFormat,
    Items->GetCount(), (Items->GetCount() > 0 ? Items->GetString(0) : UnicodeString()));
}

UnicodeString FileNameFormatString(const UnicodeString & SingleFileFormat,
  const UnicodeString & MultiFilesFormat, const Classes::TStrings * Files, bool Remote)
{
  assert(Files != nullptr);
  UnicodeString Item;
  if (Files->GetCount() > 0)
  {
    Item = Remote ? core::UnixExtractFileName(Files->GetString(0)) :
      core::ExtractFileName(Files->GetString(0), true);
  }
  return ItemsFormatString(SingleFileFormat, MultiFilesFormat,
    Files->GetCount(), Item);
}

UnicodeString UniqTempDir(const UnicodeString & BaseDir, const UnicodeString & Identity,
  bool Mask)
{
  UnicodeString TempDir;
  do
  {
    TempDir = BaseDir.IsEmpty() ? SystemTemporaryDirectory() : BaseDir;
    TempDir = Sysutils::IncludeTrailingBackslash(TempDir) + Identity;
    if (Mask)
    {
      TempDir += L"?????";
    }
    else
    {
#if defined(__BORLANDC__)
      TempDir += ::IncludeTrailingBackslash(FormatDateTime(L"nnzzz", Now()));
#else
      Classes::TDateTime dt = Classes::Now();
      uint16_t H, M, S, MS;
      dt.DecodeTime(H, M, S, MS);
      TempDir += Sysutils::IncludeTrailingBackslash(FORMAT(L"%02d%03d", M, MS));
#endif
    }
  }
  while (!Mask && Sysutils::DirectoryExists(TempDir));

  return TempDir;
}

bool DeleteDirectory(const UnicodeString & ADirName)
{
  TSearchRecChecked SearchRec;
  bool retval = true;
  if (Sysutils::FindFirst(ADirName + L"\\*", Sysutils::faAnyFile, SearchRec) == 0) // VCL Function
  {
    if (FLAGSET(SearchRec.Attr, Sysutils::faDirectory))
    {
      if ((SearchRec.Name != THISDIRECTORY) && (SearchRec.Name != PARENTDIRECTORY))
        retval = DeleteDirectory(ADirName + L"\\" + SearchRec.Name);
    }
    else
    {
      retval = Sysutils::DeleteFile(ApiPath(ADirName + L"\\" + SearchRec.Name));
    }

    if (retval)
    {
      while (FindNextChecked(SearchRec) == 0)
      { // VCL Function
        if (FLAGSET(SearchRec.Attr, Sysutils::faDirectory))
        {
          if ((SearchRec.Name != THISDIRECTORY) && (SearchRec.Name != PARENTDIRECTORY))
            retval = DeleteDirectory(ADirName + L"\\" + SearchRec.Name);
        }
        else
        {
          retval = Sysutils::DeleteFile(ApiPath(ADirName + L"\\" + SearchRec.Name));
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
    retval = Sysutils::RemoveDir(ADirName); // VCL function
  }
  return retval;
}

UnicodeString FormatDateTimeSpan(const UnicodeString & /* TimeFormat */, const Classes::TDateTime & DateTime)
{
  UnicodeString Result;
  if (static_cast<int>(DateTime) > 0)
  {
    Result = Sysutils::IntToStr(static_cast<intptr_t>((double)DateTime)) + L", ";
  }
  // days are decremented, because when there are to many of them,
  // "integer overflow" error occurs
#if defined(__BORLANDC__)
  Result += FormatDateTime(TimeFormat, DateTime - int(DateTime));
#else
  Classes::TDateTime dt(DateTime - static_cast<int>(DateTime));
  uint16_t H, M, S, MS;
  dt.DecodeTime(H, M, S, MS);
  Result += FORMAT(L"%02d:%02d:%02d", H, M, S);
#endif
  return Result;
}

TLocalCustomCommand::TLocalCustomCommand()
{
}

TLocalCustomCommand::TLocalCustomCommand(const TCustomCommandData & Data,
    const UnicodeString & APath) :
  TFileCustomCommand(Data, APath)
{
}

TLocalCustomCommand::TLocalCustomCommand(const TCustomCommandData & Data,
  const UnicodeString & APath, const UnicodeString & AFileName,
  const UnicodeString & LocalFileName, const UnicodeString & FileList) :
  TFileCustomCommand(Data, APath, AFileName, FileList)
{
  FLocalFileName = LocalFileName;
}

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

void TLocalCustomCommand::DelimitReplacement(
  UnicodeString & /*Replacement*/, wchar_t /*Quote*/)
{
  // never delimit local commands
}

bool TLocalCustomCommand::HasLocalFileName(const UnicodeString & Command)
{
  return FindPattern(Command, L'^');
}

bool TLocalCustomCommand::IsFileCommand(const UnicodeString & Command)
{
  return TFileCustomCommand::IsFileCommand(Command) || HasLocalFileName(Command);
}

