//---------------------------------------------------------------------------
#include "nbafx.h"
#define NO_WIN32_LEAN_AND_MEAN

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
bool FindFile(UnicodeString &Path)
{
    bool Result = FileExists(Path);
    if (!Result)
    {
        size_t Len = GetEnvironmentVariable(L"PATH", NULL, 0);
        if (Len > 0)
        {
            // DEBUG_PRINTF(L"Len = %d", Len);
            UnicodeString Paths;
            Paths.SetLength(Len - 1);
            GetEnvironmentVariable(L"PATH", reinterpret_cast<LPWSTR>(const_cast<wchar_t *>(Paths.c_str())), static_cast<DWORD>(Len));
            // DEBUG_PRINTF(L"Paths = %s", Paths.c_str());

            UnicodeString NewPath = FileSearch(ExtractFileName(Path, true), Paths);
            Result = !NewPath.empty();
            if (Result)
            {
                Path = NewPath;
            }
        }
    }
    // DEBUG_PRINTF(L"Result = %d", Result);
    return Result;
}
//---------------------------------------------------------------------------
bool FileExistsEx(const UnicodeString Path)
{
    UnicodeString path = Path;
    return FindFile(path);
}
//---------------------------------------------------------------------------
void OpenSessionInPutty(const UnicodeString PuttyPath,
                        TSessionData *SessionData, const UnicodeString Password)
{
    UnicodeString Program, Params, Dir;
    SplitCommand(PuttyPath, Program, Params, Dir);
    Program = ExpandEnvironmentVariables(Program);
    UnicodeString password = Password;
    if (FindFile(Program))
    {
        UnicodeString SessionName;
        TRegistryStorage *Storage = NULL;
        TSessionData *ExportData = NULL;
        TRegistryStorage *SourceStorage = NULL;
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
                            password = L"";
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
        if (!password.empty())
        {
            Params += FORMAT(L"-pw %s ", EscapePuttyCommandParam(password).c_str());
        }
        Params += FORMAT(L"-load %s", EscapePuttyCommandParam(SessionName).c_str());

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
bool ExecuteShell(const UnicodeString Path, const UnicodeString Params)
{
    return ((int)::ShellExecute(NULL, L"open", const_cast<wchar_t *>(Path.data()),
                                const_cast<wchar_t *>(Params.data()), NULL, SW_SHOWNORMAL) > 32);
}
//---------------------------------------------------------------------------
bool ExecuteShell(const UnicodeString Path, const UnicodeString Params,
                  HANDLE &Handle)
{
    // DEBUG_PRINTF(L"Path = %s, Params = %s", Path.c_str(), Params.c_str());
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
bool ExecuteShellAndWait(HINSTANCE Handle, const UnicodeString Path,
                         const UnicodeString Params, const processmessages_signal_type &ProcessMessages)
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
                    throw ExtException(LoadStr(DOCUMENT_WAIT_ERROR));
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
bool ExecuteShellAndWait(HINSTANCE Handle, const UnicodeString Command,
                         const processmessages_signal_type &ProcessMessages)
{
    UnicodeString Program, Params, Dir;
    SplitCommand(Command, Program, Params, Dir);
    return ExecuteShellAndWait(Handle, Program, Params, ProcessMessages);
}
//---------------------------------------------------------------------------
bool SpecialFolderLocation(int PathID, UnicodeString &Path)
{
    LPITEMIDLIST Pidl;
    wchar_t Buf[260];
    if (SHGetSpecialFolderLocation(NULL, PathID, &Pidl) == NO_ERROR &&
            SHGetPathFromIDList(Pidl, Buf))
    {
        Path = UnicodeString(Buf);
        return true;
    }
    return false;
}
//---------------------------------------------------------------------------
UnicodeString ItemsFormatString(const UnicodeString SingleItemFormat,
                               const UnicodeString MultiItemsFormat, size_t Count, const UnicodeString FirstItem)
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
UnicodeString ItemsFormatString(const UnicodeString SingleItemFormat,
                               const UnicodeString MultiItemsFormat, System::TStrings *Items)
{
    return ItemsFormatString(SingleItemFormat, MultiItemsFormat,
                             Items->GetCount(), (Items->GetCount() > 0 ? Items->GetString(0) : UnicodeString()));
}
//---------------------------------------------------------------------------
UnicodeString FileNameFormatString(const UnicodeString SingleFileFormat,
                                  const UnicodeString MultiFilesFormat, System::TStrings *Files, bool Remote)
{
    assert(Files != NULL);
    UnicodeString Item;
    if (Files->GetCount() > 0)
    {
        Item = Remote ? UnixExtractFileName(Files->GetString(0)) :
               ExtractFileName(Files->GetString(0), true);
    }
    return ItemsFormatString(SingleFileFormat, MultiFilesFormat,
                             Files->GetCount(), Item);
}
//---------------------------------------------------------------------
UnicodeString FormatBytes(__int64 Bytes, bool UseOrders)
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
//---------------------------------------------------------------------------
UnicodeString UniqTempDir(const UnicodeString BaseDir, const UnicodeString Identity,
                         bool Mask)
{
    UnicodeString TempDir;
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
            TempDir += IncludeTrailingBackslash(FormatDateTime(L"nnzzz", System::Now()));
        };
    }
    while (!Mask && DirectoryExists(TempDir));

    return TempDir;
}
//---------------------------------------------------------------------------
bool DeleteDirectory(const UnicodeString DirName)
{
    bool retval = true;
    WIN32_FIND_DATA sr;
    // sr.dwFileAttributes = faAnyFile;
    HANDLE h = ::FindFirstFileW(DirName.c_str(), &sr);
    if (h != INVALID_HANDLE_VALUE)
    {
        if (FLAGSET(sr.dwFileAttributes, faDirectory))
        {
            if ((wcscmp(sr.cFileName, THISDIRECTORY) != 0) && (wcscmp(sr.cFileName, PARENTDIRECTORY) != 0))
            {
                retval = ::DeleteDirectory(DirName + L"\\" + sr.cFileName);
            }
        }
        else
        {
            retval = ::DeleteFile(DirName + L"\\" + sr.cFileName);
        }

        if (retval)
        {
            while (::FindNextFile(h, &sr) == 0)
            {
                if (FLAGSET(sr.dwFileAttributes, faDirectory))
                {
                    if ((wcscmp(sr.cFileName, THISDIRECTORY) != 0) && (wcscmp(sr.cFileName, PARENTDIRECTORY) != 0))
                    {
                        retval = ::DeleteDirectory(DirName + L"\\" + sr.cFileName);
                    }
                }
                else
                {
                    retval = ::DeleteFile(DirName + L"\\" + sr.cFileName);
                }

                if (!retval) { break; }
            }
        }
    }
    ::FindClose(h);
    if (retval) { retval = ::RemoveDir(DirName); }
    return retval;
}
//---------------------------------------------------------------------------
UnicodeString FormatDateTimeSpan(const UnicodeString TimeFormat, System::TDateTime DateTime)
{
    UnicodeString Result;
    if (static_cast<int>(DateTime) > 0)
    {
        Result = IntToStr(static_cast<int>(DateTime)) + L", ";
    }
    // days are decremented, because when there are to many of them,
    // "integer overflow" error occurs
    // Result += FormatDateTime(TimeFormat, System::TDateTime(DateTime - int(DateTime)));
    // DEBUG_PRINTF(L"TimeFormat = %s", TimeFormat.c_str());
    System::TDateTime dt(DateTime - static_cast<int>(DateTime));
    unsigned int H, M, S, MS;
    dt.DecodeTime(H, M, S, MS);
    Result += FORMAT(L"%d:%02d:%02d", H, M, S);
    return Result;
}
//---------------------------------------------------------------------------
TLocalCustomCommand::TLocalCustomCommand()
{
}
//---------------------------------------------------------------------------
TLocalCustomCommand::TLocalCustomCommand(const TCustomCommandData &Data,
        const UnicodeString Path) :
    TFileCustomCommand(Data, Path)
{
}
//---------------------------------------------------------------------------
TLocalCustomCommand::TLocalCustomCommand(const TCustomCommandData &Data,
        const UnicodeString Path, const UnicodeString FileName,
        const UnicodeString LocalFileName, const UnicodeString FileList) :
    TFileCustomCommand(Data, Path, FileName, FileList)
{
    FLocalFileName = LocalFileName;
}
//---------------------------------------------------------------------------
size_t TLocalCustomCommand::PatternLen(size_t Index, char PatternCmd)
{
    size_t Len = 0;
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
bool TLocalCustomCommand::PatternReplacement(size_t Index,
        const UnicodeString Pattern, UnicodeString &Replacement, bool &Delimit)
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
void TLocalCustomCommand::DelimitReplacement(
    const UnicodeString /*Replacement*/, char /*Quote*/)
{
    // never delimit local commands
}
//---------------------------------------------------------------------------
bool TLocalCustomCommand::HasLocalFileName(const UnicodeString Command)
{
    return FindPattern(Command, '^');
}
//---------------------------------------------------------------------------
bool TLocalCustomCommand::IsFileCommand(const UnicodeString Command)
{
    return TFileCustomCommand::IsFileCommand(Command) || HasLocalFileName(Command);
}
