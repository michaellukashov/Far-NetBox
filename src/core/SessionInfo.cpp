//---------------------------------------------------------------------------
#ifndef _MSC_VER
#include <vcl.h>
#pragma hdrstop
#endif

#include "stdafx.h"

#include <stdio.h>
#include <lmcons.h>
#define SECURITY_WIN32
#include <sspi.h>
#include <secext.h>

#include "boostdefines.hpp"
#include <boost/scope_exit.hpp>
#include <boost/bind.hpp>

#include "Common.h"
#include "SessionInfo.h"
#include "Exceptions.h"
#include "TextsCore.h"
//---------------------------------------------------------------------------
#ifndef _MSC_VER
#pragma package(smart_init)
#endif
//---------------------------------------------------------------------------
std::wstring __fastcall DoXmlEscape(std::wstring Str, bool NewLine)
{
    for (size_t i = 0; i < Str.size(); i++)
    {
        const wchar_t *Repl = NULL;
        switch (Str[i])
        {
        case L'&':
            Repl = L"amp;";
            break;

        case L'>':
            Repl = L"gt;";
            break;

        case L'<':
            Repl = L"lt;";
            break;

        case L'"':
            Repl = L"quot;";
            break;

      case L'\n':
        if (NewLine)
        {
          Repl = L"#10;";
        }
        break;

        case L'\r':
            Str.erase(i, 1);
            i--;
            break;
        }

        if (Repl != NULL)
        {
            Str[i] = '&';
            Str.insert(i + 1, std::wstring(Repl));
            i += wcslen(Repl);
        }
    }
    return Str;
}
//---------------------------------------------------------------------------
std::wstring __fastcall XmlEscape(std::wstring Str)
{
    return DoXmlEscape(Str, false);
}
//---------------------------------------------------------------------------
std::wstring __fastcall XmlAttributeEscape(std::wstring Str)
{
  return DoXmlEscape(Str, true);
}
//---------------------------------------------------------------------------
std::wstring __fastcall XmlTimestamp(const nb::TDateTime DateTime)
{
    return FormatDateTime(L"yyyy'-'mm'-'dd'T'hh':'nn':'ss'.'zzz'Z'", ConvertTimestampToUTC(DateTime));
}
//---------------------------------------------------------------------------
std::wstring __fastcall XmlTimestamp()
{
    return XmlTimestamp(nb::Now());
}
//---------------------------------------------------------------------------
nb::TStrings * __fastcall ExceptionToMessages(const std::exception * E)
{
  nb::TStrings * Result = NULL;
  std::wstring Message;
  if (ExceptionMessage(E, Message))
  {
    Result = new nb::TStringList();
    Result->Add(Message);
    const ExtException * EE = dynamic_cast<const ExtException *>(E);
    if ((EE != NULL) && (EE->GetMoreMessages() != NULL))
    {
      Result->AddStrings(EE->GetMoreMessages());
    }
  }
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
#ifndef _MSC_VER
#pragma warn -inl
#endif
class TSessionActionRecord
{
public:
    TSessionActionRecord(TActionLog *Log, TLogAction Action) :
        FLog(Log),
        FAction(Action),
        FState(Opened),
        FRecursive(false),
        FErrorMessages(NULL),
        FNames(new nb::TStringList()),
        FValues(new nb::TStringList()),
        FFileList(NULL),
        FFile(NULL)
    {
        FLog->AddPendingAction(this);
    }

    ~TSessionActionRecord()
    {
        delete FErrorMessages;
        delete FNames;
        delete FValues;
        delete FFileList;
    delete FFile;
    }

    void __fastcall Restart()
    {
        FState = Opened;
        FRecursive = false;
        delete FErrorMessages;
        FErrorMessages = NULL;
        delete FFileList;
        FFileList = NULL;
        delete FFile;
        FFile = NULL;
        FNames->Clear();
        FValues->Clear();
    }

    bool __fastcall Record()
    {
        bool Result = (FState != Opened);
        if (Result)
        {
            if ((FLog->FLogging) && (FState != Cancelled))
            {
                const wchar_t *Name = ActionName();
                std::wstring Attrs;
                if (FRecursive)
                {
                    Attrs = L" recursive=\"true\"";
                }
                FLog->AddIndented(FORMAT(L"  <%s%s>", Name,  Attrs.c_str()));
                for (size_t Index = 0; Index < FNames->GetCount(); Index++)
                {
                    std::wstring Value = FValues->GetString(Index);
                    if (Value.empty())
                    {
                        FLog->AddIndented(FORMAT(L"    <%s />", FNames->GetString(Index).c_str()));
                    }
                    else
                    {
                        FLog->AddIndented(FORMAT(L"    <%s value=\"%s\" />",
                                                   FNames->GetString(Index).c_str(), XmlEscape(Value).c_str()));
                    }
                }
                if (FFileList != NULL)
                {
                    FLog->AddIndented(L"    <files>");
                    for (size_t Index = 0; Index < FFileList->GetCount(); Index++)
                    {
                        TRemoteFile *File = FFileList->GetFile(Index);

                        FLog->AddIndented(L"      <file>");
                        FLog->AddIndented(FORMAT(L"        <filename value=\"%s\" />", XmlEscape(File->GetFileName()).c_str()));
                        FLog->AddIndented(FORMAT(L"        <type value=\"%s\" />", XmlEscape(std::wstring(File->GetType(), 1)).c_str()));
                        if (!File->GetIsDirectory())
                        {
                            FLog->AddIndented(FORMAT(L"        <size value=\"%s\" />", Int64ToStr(File->GetSize()).c_str()));
                        }
                        FLog->AddIndented(FORMAT(L"        <modification value=\"%s\" />", XmlTimestamp(File->GetModification()).c_str()));
                        FLog->AddIndented(FORMAT(L"        <permissions value=\"%s\" />", XmlEscape(File->GetRights()->GetText()).c_str()));
                        FLog->AddIndented(L"      </file>");
                    }
                    FLog->AddIndented(L"    </files>");
                }
        if (FFile != NULL)
        {
          FLog->AddIndented(L"  <file>");
          FLog->AddIndented(FORMAT(L"    <type value=\"%s\" />", XmlAttributeEscape(std::wstring(1, FFile->GetType())).c_str()));
          if (!FFile->GetIsDirectory())
          {
            FLog->AddIndented(FORMAT(L"    <size value=\"%s\" />", IntToStr(FFile->GetSize()).c_str()));
          }
          FLog->AddIndented(FORMAT(L"    <modification value=\"%s\" />", XmlTimestamp(FFile->GetModification()).c_str()));
          FLog->AddIndented(FORMAT(L"    <permissions value=\"%s\" />", XmlAttributeEscape(FFile->GetRights()->GetText()).c_str()));
          FLog->AddIndented(L"  </file>");
        }
                if (FState == RolledBack)
                {
                    if (FErrorMessages != NULL)
                    {
                        FLog->AddIndented(L"    <result success=\"false\">");
                        FLog->AddMessages(L"    ", FErrorMessages);
                        FLog->AddIndented(L"    </result>");
                    }
                    else
                    {
                        FLog->AddIndented(L"    <result success=\"false\" />");
                    }
                }
                else
                {
                    FLog->AddIndented(L"    <result success=\"true\" />");
                }
                FLog->AddIndented(FORMAT(L"  </%s>", Name));
            }
            delete this;
        }
        return Result;
    }

    void __fastcall Commit()
    {
        Close(Committed);
    }

    void __fastcall Rollback(const std::exception *E)
    {
        assert(FErrorMessages == NULL);
        FErrorMessages = ExceptionToMessages(E);
/*
        if (E->what() && *E->what())
        {
            FErrorMessages->Add(nb::MB2W(E->what()));
        }
        const ExtException *EE = dynamic_cast<const ExtException *>(E);
        if ((EE != NULL) && (EE->GetMoreMessages() != NULL))
        {
            FErrorMessages->AddStrings(EE->GetMoreMessages());
        }
        if (FErrorMessages->GetCount() == 0)
        {
            delete FErrorMessages;
            FErrorMessages = NULL;
        }
*/
        Close(RolledBack);
    }

    void __fastcall Cancel()
    {
        Close(Cancelled);
    }

    void __fastcall FileName(const std::wstring FileName)
    {
        Parameter(L"filename", FileName);
    }

    void __fastcall Destination(const std::wstring Destination)
    {
        Parameter(L"destination", Destination);
    }

    void __fastcall Rights(const TRights &Rights)
    {
        Parameter(L"permissions", Rights.GetText());
    }

    void __fastcall Modification(const nb::TDateTime DateTime)
    {
        Parameter(L"modification", XmlTimestamp(DateTime));
    }

    void __fastcall Recursive()
    {
        FRecursive = true;
    }

    void __fastcall Command(const std::wstring Command)
    {
        Parameter(L"command", Command);
    }

    void __fastcall AddOutput(const std::wstring Output, bool StdError)
    {
        const wchar_t *Name = (StdError ? L"erroroutput" : L"output");
        size_t Index = FNames->IndexOf(Name);
        if (Index != NPOS)
        {
            FValues->PutString(Index, FValues->GetString(Index) + L"\r\n" + Output);
        }
        else
        {
            Parameter(Name, Output);
        }
    }

    void __fastcall FileList(TRemoteFileList *FileList)
    {
        if (FFileList == NULL)
        {
            FFileList = new TRemoteFileList();
        }
        FileList->DuplicateTo(FFileList);
    }

  void __fastcall File(TRemoteFile * File)
  {
    if (FFile != NULL)
    {
      delete FFile;
    }
    FFile = File->Duplicate(true);
  }

protected:
    enum TState { Opened, Committed, RolledBack, Cancelled };

    inline void __fastcall Close(TState State)
    {
        assert(FState == Opened);
        FState = State;
        FLog->RecordPendingActions();
    }

    const wchar_t * __fastcall ActionName()
    {
        switch (FAction)
        {
        case laUpload: return L"upload";
        case laDownload: return L"download";
        case laTouch: return L"touch";
        case laChmod: return L"chmod";
        case laMkdir: return L"mkdir";
        case laRm: return L"rm";
        case laMv: return L"mv";
        case laCall: return L"call";
        case laLs: return L"ls";
        case laStat: return L"stat";
        default: assert(false); return L"";
        }
    }

    void __fastcall Parameter(const std::wstring Name, const std::wstring Value = L"")
    {
        FNames->Add(Name);
        FValues->Add(Value);
    }

private:
    TActionLog *FLog;
    TLogAction FAction;
    TState FState;
    bool FRecursive;
    nb::TStrings *FErrorMessages;
    nb::TStrings *FNames;
    nb::TStrings *FValues;
    TRemoteFileList *FFileList;
  TRemoteFile * FFile;
};
#ifndef _MSC_VER
#pragma warn .inl
#endif
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TSessionAction::TSessionAction(TActionLog *Log, TLogAction Action)
{
    if (Log->FLogging)
    {
        FRecord = new TSessionActionRecord(Log, Action);
    }
    else
    {
        FRecord = NULL;
    }
}
//---------------------------------------------------------------------------
TSessionAction::~TSessionAction()
{
    if (FRecord != NULL)
    {
        Commit();
    }
}
//---------------------------------------------------------------------------
void __fastcall TSessionAction::Restart()
{
    if (FRecord != NULL)
    {
        FRecord->Restart();
    }
}
//---------------------------------------------------------------------------
void __fastcall TSessionAction::Commit()
{
    if (FRecord != NULL)
    {
        TSessionActionRecord *Record = FRecord;
        FRecord = NULL;
        Record->Commit();
    }
}
//---------------------------------------------------------------------------
void __fastcall TSessionAction::Rollback(const std::exception *E)
{
    if (FRecord != NULL)
    {
        TSessionActionRecord *Record = FRecord;
        FRecord = NULL;
        Record->Rollback(E);
    }
}
//---------------------------------------------------------------------------
void __fastcall TSessionAction::Cancel()
{
    if (FRecord != NULL)
    {
        TSessionActionRecord *Record = FRecord;
        FRecord = NULL;
        Record->Cancel();
    }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFileSessionAction::TFileSessionAction(TActionLog *Log, TLogAction Action) :
    TSessionAction(Log, Action)
{
};
//---------------------------------------------------------------------------
TFileSessionAction::TFileSessionAction(
    TActionLog *Log, TLogAction Action, const std::wstring AFileName) :
    TSessionAction(Log, Action)
{
    FileName(AFileName);
};
//---------------------------------------------------------------------------
void __fastcall TFileSessionAction::FileName(const std::wstring FileName)
{
    if (FRecord != NULL)
    {
        FRecord->FileName(FileName);
    }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TFileLocationSessionAction::TFileLocationSessionAction(
    TActionLog *Log, TLogAction Action) :
    TFileSessionAction(Log, Action)
{
};
//---------------------------------------------------------------------------
TFileLocationSessionAction::TFileLocationSessionAction(
    TActionLog *Log, TLogAction Action, const std::wstring FileName) :
    TFileSessionAction(Log, Action, FileName)
{
};
//---------------------------------------------------------------------------
void __fastcall TFileLocationSessionAction::Destination(const std::wstring Destination)
{
    if (FRecord != NULL)
    {
        FRecord->Destination(Destination);
    }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TUploadSessionAction::TUploadSessionAction(TActionLog *Log) :
    TFileLocationSessionAction(Log, laUpload)
{
};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TDownloadSessionAction::TDownloadSessionAction(TActionLog *Log) :
    TFileLocationSessionAction(Log, laDownload)
{
};
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TChmodSessionAction::TChmodSessionAction(
    TActionLog *Log, const std::wstring FileName) :
    TFileSessionAction(Log, laChmod, FileName)
{
}
//---------------------------------------------------------------------------
void TChmodSessionAction::Recursive()
{
    if (FRecord != NULL)
    {
        FRecord->Recursive();
    }
}
//---------------------------------------------------------------------------
TChmodSessionAction::TChmodSessionAction(
    TActionLog *Log, const std::wstring FileName, const TRights &ARights) :
    TFileSessionAction(Log, laChmod, FileName)
{
    Rights(ARights);
}
//---------------------------------------------------------------------------
void __fastcall TChmodSessionAction::Rights(const TRights &Rights)
{
    if (FRecord != NULL)
    {
        FRecord->Rights(Rights);
    }
}
//---------------------------------------------------------------------------
TTouchSessionAction::TTouchSessionAction(
    TActionLog *Log, const std::wstring FileName, const nb::TDateTime &Modification) :
    TFileSessionAction(Log, laTouch, FileName)
{
    if (FRecord != NULL)
    {
        FRecord->Modification(Modification);
    }
}
//---------------------------------------------------------------------------
TMkdirSessionAction::TMkdirSessionAction(
    TActionLog *Log, const std::wstring FileName) :
    TFileSessionAction(Log, laMkdir, FileName)
{
}
//---------------------------------------------------------------------------
TRmSessionAction::TRmSessionAction(
    TActionLog *Log, const std::wstring FileName) :
    TFileSessionAction(Log, laRm, FileName)
{
}
//---------------------------------------------------------------------------
void __fastcall TRmSessionAction::Recursive()
{
    if (FRecord != NULL)
    {
        FRecord->Recursive();
    }
}
//---------------------------------------------------------------------------
TMvSessionAction::TMvSessionAction(TActionLog *Log,
                                   const std::wstring FileName, const std::wstring ADestination) :
    TFileLocationSessionAction(Log, laMv, FileName)
{
    Destination(ADestination);
}
//---------------------------------------------------------------------------
TCallSessionAction::TCallSessionAction(TActionLog *Log,
                                       const std::wstring Command, const std::wstring Destination) :
    TSessionAction(Log, laCall)
{
    if (FRecord != NULL)
    {
        FRecord->Command(Command);
        FRecord->Destination(Destination);
    }
}
//---------------------------------------------------------------------------
void __fastcall TCallSessionAction::AddOutput(const std::wstring Output, bool StdError)
{
    if (FRecord != NULL)
    {
        FRecord->AddOutput(Output, StdError);
    }
}
//---------------------------------------------------------------------------
TLsSessionAction::TLsSessionAction(TActionLog *Log,
                                   const std::wstring Destination) :
    TSessionAction(Log, laLs)
{
    if (FRecord != NULL)
    {
        FRecord->Destination(Destination);
    }
}
//---------------------------------------------------------------------------
void __fastcall TLsSessionAction::FileList(TRemoteFileList *FileList)
{
    if (FRecord != NULL)
    {
        FRecord->FileList(FileList);
    }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TStatSessionAction::TStatSessionAction(TActionLog * Log, const std::wstring & FileName) :
  TFileSessionAction(Log, laStat, FileName)
{
}
//---------------------------------------------------------------------------
void __fastcall TStatSessionAction::File(TRemoteFile * File)
{
  if (FRecord != NULL)
  {
    FRecord->File(File);
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TSessionInfo::TSessionInfo()
{
    LoginTime = nb::Now();
}
//---------------------------------------------------------------------------
TFileSystemInfo::TFileSystemInfo()
{
    memset(&IsCapable, false, sizeof(IsCapable));
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
FILE * __fastcall OpenFile(std::wstring LogFileName, TSessionData * SessionData, bool Append, std::wstring & NewFileName)
{
  FILE * Result;
  std::wstring ANewFileName = StripPathQuotes(ExpandEnvironmentVariables(LogFileName));
  nb::TDateTime N = nb::Now();
/*
        // FFile = _wfopen(NewFileName.c_str(),
        // FConfiguration->GetLogFileAppend() && !FLogging ? L"a" : L"w");
        FFile = _fsopen(nb::W2MB(NewFileName.c_str()).c_str(),
                        FConfiguration->GetLogFileAppend() && !FLogging ? "a" : "w", SH_DENYWR);
        if (FFile)
        {
            setvbuf(static_cast<FILE *>(FFile), NULL, _IONBF, BUFSIZ);
            FCurrentFileName = NewFileName;
        }
        else
        {
            throw ExtException(FMTLOAD(LOG_OPENERROR, NewFileName.c_str()));
        }

        std::wstring NewFileName = StripPathQuotes(ExpandEnvironmentVariables(FCurrentLogFileName));
        SYSTEMTIME t;
        ::GetLocalTime(&t);
        for (size_t Index = 0; Index < NewFileName.size(); Index++)
        {
            if ((NewFileName[Index] == '&') && (Index < NewFileName.size() - 1))
            {
                std::wstring Replacement;
                // keep consistent with TFileCustomCommand::PatternReplacement
                switch (tolower(NewFileName[Index + 1]))
                {
                case 'y':
                    Replacement = FORMAT(L"%04d", t.wYear);
                    break;

                case 'm':
                    Replacement = FORMAT(L"%02d", t.wMonth);
                    break;

                case 'd':
                    Replacement = FORMAT(L"%02d", t.wDay);
                    break;

                case 't':
                    Replacement = FORMAT(L"%02d%02d%02d", t.wHour, t.wMinute, t.wSecond);
                    break;

                case '@':
                    Replacement = MakeValidFileName(FSessionData->GetHostName());
                    break;

                case 's':
                    Replacement = MakeValidFileName(FSessionData->GetSessionName());
                    break;

                case '&':
                    Replacement = L"&";
                    break;

                default:
                    Replacement = std::wstring(L"&") + NewFileName[Index + 1];
                    break;
                }
                NewFileName.erase(Index, 2);
                NewFileName.insert(Index, Replacement);
                Index += Replacement.size() - 1;
            }
        }
*/
  for (int Index = 1; Index < ANewFileName.size(); Index++)
  {
    if (ANewFileName[Index] == L'!')
    {
      std::wstring Replacement;
      // keep consistent with TFileCustomCommand::PatternReplacement
      switch (tolower(ANewFileName[Index + 1]))
      {
        case L'y':
          Replacement = FormatDateTime(L"yyyy", N);
          break;

        case L'm':
          Replacement = FormatDateTime(L"mm", N);
          break;

        case L'd':
          Replacement = FormatDateTime(L"dd", N);
          break;

        case L't':
          Replacement = FormatDateTime(L"hhnnss", N);
          break;

        case L'@':
          Replacement = MakeValidFileName(SessionData->GetHostNameExpanded());
          break;

        case L's':
          Replacement = MakeValidFileName(SessionData->GetSessionName());
          break;

        case L'!':
          Replacement = L"!";
          break;

        default:
          Replacement = std::wstring(L"!") + ANewFileName[Index + 1];
          break;
      }
      ANewFileName.erase(Index, 2);
      ANewFileName.insert(Index, Replacement);
      Index += Replacement.size() - 1;
    }
  }
  Result = _wfopen(ANewFileName.c_str(), (Append ? L"a" : L"w"));
  if (Result != NULL)
  {
    setvbuf(Result, NULL, _IONBF, BUFSIZ);
    NewFileName = ANewFileName;
  }
  else
  {
    throw ExtException(FMTLOAD(LOG_OPENERROR, ANewFileName.c_str()));
  }
  return Result;
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
const wchar_t *LogLineMarks = L"<>!.*";

TSessionLog::TSessionLog(TSessionUI *UI, TSessionData *SessionData,
                         TConfiguration *Configuration):
    nb::TStringList()
{
    FCriticalSection = new TCriticalSection;
    FConfiguration = Configuration;
    FParent = NULL;
    FUI = UI;
    FSessionData = SessionData;
    FFile = NULL;
    FLoggedLines = 0;
    FTopIndex = NPOS;
    FCurrentLogFileName = L"";
    FCurrentFileName = L"";
    FClosed = false;
    Self = this;
}
//---------------------------------------------------------------------------
TSessionLog::~TSessionLog()
{
    FClosed = true;
    ReflectSettings();
    assert(FFile == NULL);
    delete FCriticalSection;
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::Lock()
{
    FCriticalSection->Enter();
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::Unlock()
{
    FCriticalSection->Leave();
}
//---------------------------------------------------------------------------
std::wstring __fastcall TSessionLog::GetSessionName()
{
    assert(FSessionData != NULL);
    return FSessionData->GetSessionName();
}
//---------------------------------------------------------------------------
std::wstring __fastcall TSessionLog::GetLine(size_t Index)
{
    return GetString(Index - FTopIndex);
}
//---------------------------------------------------------------------------
TLogLineType __fastcall TSessionLog::GetType(size_t Index)
{
    void *ptr = GetObject(Index - FTopIndex);
    return static_cast<TLogLineType>(reinterpret_cast<size_t>(ptr));
}
//---------------------------------------------------------------------------
void TSessionLog::DoAddToParent(TLogLineType Type, const std::wstring Line)
{
    assert(FParent != NULL);
    FParent->Add(Type, Line);
}
//---------------------------------------------------------------------------
void TSessionLog::DoAddToSelf(TLogLineType Type, const std::wstring Line)
{
    if (static_cast<int>(FTopIndex) < 0)
    {
        FTopIndex = 0;
    }

    nb::TStringList::AddObject(Line, static_cast<nb::TObject *>(reinterpret_cast<void *>(static_cast<size_t>(Type))));

    FLoggedLines++;

    if (LogToFile())
    {
        if (FFile == NULL)
        {
            OpenLogFile();
        }

        if (FFile != NULL)
        {
/*
            if (Type != llAction)
            {
                SYSTEMTIME t;
                ::GetLocalTime(&t);
                // std::wstring Timestamp = FormatDateTime(L" yyyy-mm-dd hh:nn:ss.zzz ", nb::Now());
                std::wstring Timestamp = FORMAT(L" %04d-%02d-%02d %02d:%02d:%02d.%03d ",
                                                t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
                fputc(LogLineMarks[Type], static_cast<FILE *>(FFile));
                // fwrite(Timestamp.c_str(), 1, Timestamp.size() * sizeof(wchar_t), (FILE *)FFile);
                fprintf_s(static_cast<FILE *>(FFile), "%s", const_cast<char *>(nb::W2MB(Timestamp.c_str()).c_str()));
            }
            // use fwrite instead of fprintf to make sure that even
            // non-ascii data (unicode) gets in.
            fprintf_s(static_cast<FILE *>(FFile), "%s", const_cast<char *>(nb::W2MB(Line.c_str()).c_str()));
            fputc('\n', static_cast<FILE *>(FFile));
*/
            // std::wstring Timestamp = FormatDateTime(L" yyyy-mm-dd hh:nn:ss.zzz ", nb::Now());
            SYSTEMTIME t;
            ::GetLocalTime(&t);
            std::wstring Timestamp = FORMAT(L" %04d-%02d-%02d %02d:%02d:%02d.%03d ",
                                            t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond, t.wMilliseconds);
            // UTF8String UtfLine = UTF8String(UnicodeString(LogLineMarks[Type]) + Timestamp + Line + "\n");
            UTF8String UtfLine = UTF8String(UnicodeString(LogLineMarks[Type]) + Timestamp + Line + L"\n");
            fwrite(UtfLine.c_str(), UtfLine.Length(), 1, (FILE *)FFile);
            // nb::Error(SNotImplemented, 1490);
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::DoAdd(TLogLineType Type, const std::wstring Line,
                        const doaddlog_slot_type &func)
{
    std::wstring Prefix;
    std::wstring line = Line;

    if (!GetName().empty())
    {
        Prefix = L"[" + GetName() + L"] ";
    }
    doaddlog_signal_type sig;
    sig.connect(func);
    while (!line.empty())
    {
        sig(Type, Prefix + CutToChar(line, '\n', false));
    }
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::Add(TLogLineType Type, const std::wstring Line)
{
    assert(FConfiguration);
    if (GetLogging())
    {
        try
        {
            if (FParent != NULL)
            {
                DoAdd(Type, Line, boost::bind(&TSessionLog::DoAddToParent, this, _1, _2));
            }
            else
            {
                TGuard Guard(FCriticalSection);

                BeginUpdate();
                {
                    BOOST_SCOPE_EXIT ( (&Self) )
                    {
                        Self->DeleteUnnecessary();
                        Self->EndUpdate();
                    } BOOST_SCOPE_EXIT_END
                    DoAdd(Type, Line, boost::bind(&TSessionLog::DoAddToSelf, this, _1, _2));
                }
            }
        }
        catch (const std::exception &E)
        {
            // We failed logging, turn it off and notify user.
            FConfiguration->SetLogging(false);
            try
            {
                throw ExtException(FMTLOAD(LOG_GEN_ERROR), &E);
            }
            catch (const std::exception &E)
            {
                AddException(&E);
                FUI->HandleExtendedException(&E);
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::AddException(const std::exception *E)
{
    if (E != NULL)
    {
        Add(llException, ExceptionLogString(E));
    }
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::ReflectSettings()
{
    TGuard Guard(FCriticalSection);

    bool ALogging =
        !FClosed &&
    ((FParent != NULL) || FConfiguration->GetLogging());

  if (FLogging != ALogging)
    {
        FLogging = ALogging;
        StateChange();
    }

  // if logging to file was turned off or log file was changed -> close current log file
    if ((FFile != NULL) &&
      (!LogToFile() || (FCurrentLogFileName != FConfiguration->GetLogFileName())))
    {
        CloseLogFile();
    }

    DeleteUnnecessary();
}
//---------------------------------------------------------------------------
bool __fastcall TSessionLog::LogToFile()
{
    return GetLogging() && FConfiguration->GetLogToFile() && (FParent == NULL);
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::CloseLogFile()
{
    if (FFile != NULL)
    {
        fclose(static_cast<FILE *>(FFile));
        FFile = NULL;
    }
    FCurrentLogFileName = L"";
    FCurrentFileName = L"";
    StateChange();
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::OpenLogFile()
{
    try
    {
        assert(FFile == NULL);
        assert(FConfiguration != NULL);
        FCurrentLogFileName = FConfiguration->GetLogFileName();
        FFile = OpenFile(FCurrentLogFileName, FSessionData, FConfiguration->GetLogFileAppend(), FCurrentFileName);
    }
    catch (const std::exception &E)
    {
        // We failed logging to file, turn it off and notify user.
        FCurrentLogFileName = L"";
        FCurrentFileName = L"";
        FConfiguration->SetLogToFile(false);
        FConfiguration->SetLogFileName(std::wstring());
        try
        {
            throw ExtException(FMTLOAD(LOG_GEN_ERROR), &E);
        }
        catch (const std::exception &E)
        {
            AddException(&E);
            FUI->HandleExtendedException(&E);
        }
    }
    StateChange();
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::StateChange()
{
    if (!FOnStateChange.empty())
    {
        FOnStateChange(this);
    }
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::DeleteUnnecessary()
{
    BeginUpdate();
    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
            Self->EndUpdate();
        } BOOST_SCOPE_EXIT_END
        if (!GetLogging() || (FParent != NULL))
        {
            Clear();
        }
        else
        {
            while (!FConfiguration->GetLogWindowComplete() && (static_cast<int>(GetCount()) > FConfiguration->GetLogWindowLines()))
            {
                Delete(0);
                FTopIndex++;
            }
        }
    }
}
//---------------------------------------------------------------------------
void __fastcall TSessionLog::AddStartupInfo()
{
    if (GetLogging())
    {
        if (FParent != NULL)
        {
            // do not add session info for secondary session
            // (this should better be handled in the TSecondaryTerminal)
        }
        else
        {
            DoAddStartupInfo(FSessionData);
        }
    }
}
//---------------------------------------------------------------------------
void TSessionLog::DoAddStartupInfo(TSessionData *Data)
{
    TGuard Guard(FCriticalSection);

    BeginUpdate();
    {
        BOOST_SCOPE_EXIT ( (&Self) )
        {
            Self->DeleteUnnecessary();

            Self->EndUpdate();
        } BOOST_SCOPE_EXIT_END
        // #define ADF(S, ...) DoAdd(llMessage, FORMAT(S, __VA_ARGS__), boost::bind(&TSessionLog::DoAddToSelf, this, _1, _2));
        #define ADF(S, ...) DoAdd(llMessage, FORMAT(S, __VA_ARGS__), boost::bind(&TSessionLog::DoAddToSelf, this, _1, _2));
        AddSeparator();
        ADF(L"NetBox %s (OS %s)", FConfiguration->GetVersionStr().c_str(), FConfiguration->GetOSVersionStr().c_str());
        THierarchicalStorage *Storage = FConfiguration->CreateStorage();
        {
            BOOST_SCOPE_EXIT ( (&Storage) )
            {
                delete Storage;
            } BOOST_SCOPE_EXIT_END
            ADF(L"Configuration: %s", Storage->GetSource().c_str());
        }

        typedef BOOL (WINAPI *TGetUserNameEx)(EXTENDED_NAME_FORMAT NameFormat, LPSTR lpNameBuffer, PULONG nSize);
        HINSTANCE Secur32 = LoadLibrary(L"secur32.dll");
        TGetUserNameEx GetUserNameEx =
            (Secur32 != NULL) ? reinterpret_cast<TGetUserNameEx>(GetProcAddress(Secur32, "GetUserNameEx")) : NULL;
        wchar_t UserName[UNLEN + 1];
        unsigned long UserNameSize = LENOF(UserName);
        if ((GetUserNameEx == NULL) || !GetUserNameEx(NameSamCompatible, (LPSTR)UserName, &UserNameSize))
        {
            wcscpy(UserName, L"<Failed to retrieve username>");
        }
        ADF(L"Local account: %s", UserName);
        ADF(L"Login time: %s", FormatDateTime(L"dddddd tt", nb::Now()).c_str());
        AddSeparator();
        ADF(L"Session name: %s (%s)", Data->GetSessionName().c_str(), Data->GetSource().c_str());
        ADF(L"Host name: %s (Port: %d)", Data->GetHostNameExpanded().c_str(), Data->GetPortNumber());
        ADF(L"User name: %s (Password: %s, Key file: %s)",
            Data->GetUserNameExpanded().c_str(), BooleanToEngStr(!Data->GetPassword().empty()).c_str(),
            BooleanToEngStr(!Data->GetPublicKeyFile().empty()).c_str())
        ADF(L"Tunnel: %s", BooleanToEngStr(Data->GetTunnel()).c_str());
        if (Data->GetTunnel())
        {
            ADF(L"Tunnel: Host name: %s (Port: %d)", Data->GetTunnelHostName().c_str(), Data->GetTunnelPortNumber());
            ADF(L"Tunnel: User name: %s (Password: %s, Key file: %s)",
                Data->GetTunnelUserName().c_str(), BooleanToEngStr(!Data->GetTunnelPassword().empty()).c_str(),
                BooleanToEngStr(!Data->GetTunnelPublicKeyFile().empty()).c_str());
            ADF(L"Tunnel: Local port number: %d", Data->GetTunnelLocalPortNumber());
        }
        ADF(L"Transfer Protocol: %s", Data->GetFSProtocolStr().c_str());
        ADF(L"Code Page: %d", Data->GetCodePageAsNumber());
        wchar_t *PingTypes = L"-NC";
        TPingType PingType;
        size_t PingInterval;
        if ((Data->GetFSProtocol() == fsFTP) || (Data->GetFSProtocol() == fsFTPS))
        {
            PingType = Data->GetFtpPingType();
            PingInterval = Data->GetFtpPingInterval();
        }
        else
        {
            PingType = Data->GetPingType();
            PingInterval = Data->GetPingInterval();
        }
        ADF(L"Ping type: %c, Ping interval: %d sec; Timeout: %d sec",
            PingTypes[PingType], PingInterval, Data->GetTimeout());
        ADF(L"Proxy: %s", ProxyMethodList[Data->GetProxyMethod()]);
        if (Data->GetProxyMethod() != pmNone)
        {
            ADF(L"HostName: %s (Port: %d); Username: %s; Passwd: %s",
                Data->GetProxyHost().c_str(), Data->GetProxyPort(),
                Data->GetProxyUsername().c_str(), BooleanToEngStr(!Data->GetProxyPassword().empty()).c_str());
            if (Data->GetProxyMethod() == pmTelnet)
            {
                ADF(L"Telnet command: %s", Data->GetProxyTelnetCommand().c_str());
            }
            if (Data->GetProxyMethod() == pmCmd)
            {
                ADF(L"Local command: %s", Data->GetProxyLocalCommand().c_str());
            }
        }
        if (Data->GetUsesSsh())
        {
            ADF(L"SSH protocol version: %s; Compression: %s",
                Data->GetSshProtStr().c_str(), BooleanToEngStr(Data->GetCompression()).c_str());
            ADF(L"Bypass authentication: %s",
                BooleanToEngStr(Data->GetSshNoUserAuth()).c_str());
            ADF(L"Try agent: %s; Agent forwarding: %s; TIS/CryptoCard: %s; KI: %s; GSSAPI: %s",
                BooleanToEngStr(Data->GetTryAgent()).c_str(),
                BooleanToEngStr(Data->GetAgentFwd()).c_str(),
                BooleanToEngStr(Data->GetAuthTIS()).c_str(),
                BooleanToEngStr(Data->GetAuthKI()).c_str(),
                BooleanToEngStr(Data->GetAuthGSSAPI()).c_str());
            if (Data->GetAuthGSSAPI())
            {
                ADF(L"GSSAPI: Forwarding: %s; Server realm: %s",
                    BooleanToEngStr(Data->GetGSSAPIFwdTGT()).c_str(), Data->GetGSSAPIServerRealm().c_str());
            }
            ADF(L"Ciphers: %s; Ssh2DES: %s",
                Data->GetCipherList().c_str(), BooleanToEngStr(Data->GetSsh2DES()).c_str());
            std::wstring Bugs;
            wchar_t const *BugFlags = L"A+-";
            for (int Index = 0; Index < BUG_COUNT; Index++)
            {
                Bugs += BugFlags[Data->GetBug(static_cast<TSshBug>(Index))];
                Bugs += Index<BUG_COUNT-1? L"," : L"";
            }
            ADF(L"SSH Bugs: %s", Bugs.c_str());
            Bugs = L"";
            for (int Index = 0; Index < SFTP_BUG_COUNT; Index++)
            {
                Bugs += BugFlags[Data->GetSFTPBug(static_cast<TSftpBug>(Index))];
                Bugs += Index<SFTP_BUG_COUNT-1 ? L"," : L"";
            }
            ADF(L"SFTP Bugs: %s", Bugs.c_str());
            ADF(L"Return code variable: %s; Lookup user groups: %s",
                Data->GetDetectReturnVar() ? std::wstring(L"Autodetect").c_str() : Data->GetReturnVar().c_str(),
                BooleanToEngStr(Data->GetLookupUserGroups()).c_str());
            ADF(L"Shell: %s", Data->GetShell().empty() ? std::wstring(L"default").c_str() : Data->GetShell().c_str());
            ADF(L"EOL: %d, UTF: %d", Data->GetEOLType(), Data->GetNotUtf());
            ADF(L"Clear aliases: %s, Unset nat.vars: %s, Resolve symlinks: %s",
                BooleanToEngStr(Data->GetClearAliases()).c_str(),
                BooleanToEngStr(Data->GetUnsetNationalVars()).c_str(),
                BooleanToEngStr(Data->GetResolveSymlinks()).c_str());
            ADF(L"LS: %s, Ign LS warn: %s, Scp1 Comp: %s",
                Data->GetListingCommand().c_str(),
                BooleanToEngStr(Data->GetIgnoreLsWarnings()).c_str(),
                BooleanToEngStr(Data->GetScp1Compatibility()).c_str());
        }
        if ((Data->GetFSProtocol() == fsFTP) || (Data->GetFSProtocol() == fsFTPS))
        {
            std::wstring Ftps;
            switch (Data->GetFtps())
            {
            case ftpsImplicit:
                Ftps = L"Implicit SSL/TLS";
                break;

            case ftpsExplicitSsl:
                Ftps = L"Explicit SSL";
                break;

            case ftpsExplicitTls:
                Ftps = L"Explicit TLS";
                break;

            default:
                assert(Data->GetFtps() == ftpsNone);
                Ftps = L"None";
                break;
            }
            ADF(L"FTP: FTPS: %s; Passive: %s [Force IP: %s]",
                Ftps.c_str(), BooleanToEngStr(Data->GetFtpPasvMode()).c_str(),
                BooleanToEngStr(Data->GetFtpForcePasvIp()).c_str());
        }
        ADF(L"Local directory: %s, Remote directory: %s, Update: %s, Cache: %s",
            (Data->GetLocalDirectory().empty() ? std::wstring(L"default").c_str() : Data->GetLocalDirectory().c_str()),
            (Data->GetRemoteDirectory().empty() ? std::wstring(L"home").c_str() : Data->GetRemoteDirectory().c_str()),
            BooleanToEngStr(Data->GetUpdateDirectories()).c_str(),
            BooleanToEngStr(Data->GetCacheDirectories()).c_str());
        ADF(L"Cache directory changes: %s, Permanent: %s",
            BooleanToEngStr(Data->GetCacheDirectoryChanges()).c_str(),
            BooleanToEngStr(Data->GetPreserveDirectoryChanges()).c_str());
        ADF(L"DST mode: %d", static_cast<int>(Data->GetDSTMode()));

        AddSeparator();

#undef ADF
    }
}
//---------------------------------------------------------------------------
void TSessionLog::AddSeparator()
{
    Add(llMessage, L"--------------------------------------------------------------------------");
}
//---------------------------------------------------------------------------
size_t TSessionLog::GetBottomIndex()
{
    return (GetCount() > 0 ? (GetTopIndex() + GetCount() - 1) : -1);
}
//---------------------------------------------------------------------------
bool TSessionLog::GetLoggingToFile()
{
    assert((FFile == NULL) || LogToFile());
    return (FFile != NULL);
}
//---------------------------------------------------------------------------
void TSessionLog::Clear()
{
    TGuard Guard(FCriticalSection);

    FTopIndex += GetCount();
    nb::TStringList::Clear();
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TActionLog::TActionLog(TSessionUI* UI, TSessionData * SessionData,
  TConfiguration * Configuration)
{
  FCriticalSection = new TCriticalSection;
  FConfiguration = Configuration;
  FUI = UI;
  FSessionData = SessionData;
  FFile = NULL;
  FCurrentLogFileName = L"";
  FCurrentFileName = L"";
  FLogging = false;
  FClosed = false;
  FPendingActions = new nb::TList();
  FIndent = L"  ";
  FInGroup = false;
  FEnabled = true;
}
//---------------------------------------------------------------------------
TActionLog::~TActionLog()
{
  assert(FPendingActions->GetCount() == 0);
  delete FPendingActions;
  FClosed = true;
  ReflectSettings();
  assert(FFile == NULL);
  delete FCriticalSection;
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::Add(const std::wstring & Line)
{
  assert(FConfiguration);
  if (FLogging)
  {
    try
    {
      TGuard Guard(FCriticalSection);
      if (FFile == NULL)
      {
        OpenLogFile();
      }

      if (FFile != NULL)
      {
        // UTF8String UtfLine = UTF8String(Line);
        // fwrite(UtfLine.c_str(), UtfLine.Length(), 1, (FILE *)FFile);
        // fwrite("\n", 1, 1, (FILE *)FFile);
        nb::Error(SNotImplemented, 1491);
      }
    }
    catch (const std::exception &E)
    {
      // We failed logging, turn it off and notify user.
      FConfiguration->SetLogActions(false);
      try
      {
        throw ExtException(FMTLOAD(LOG_GEN_ERROR), &E);
      }
      catch (const std::exception &E)
      {
        FUI->HandleExtendedException(&E);
      }
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::AddIndented(const std::wstring & Line)
{
  Add(FIndent + Line);
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::AddFailure(nb::TStrings * Messages)
{
  AddIndented(L"<failure>");
  AddMessages(L"  ", Messages);
  AddIndented(L"</failure>");
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::AddFailure(const std::exception * E)
{
  nb::TStrings * Messages = ExceptionToMessages(E);
  if (Messages != NULL)
  {
    AddFailure(Messages);
  }
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::AddMessages(std::wstring Indent, nb::TStrings * Messages)
{
  for (int Index = 0; Index < Messages->GetCount(); Index++)
  {
    AddIndented(
      FORMAT((Indent + L"<message>%s</message>").c_str(), XmlEscape(Messages->GetString(Index)).c_str()));
  }
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::ReflectSettings()
{
  TGuard Guard(FCriticalSection);

  bool ALogging =
    !FClosed && FConfiguration->GetLogActions() && GetEnabled();

  if (ALogging && !FLogging)
  {
    FLogging = true;
    Add(L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    Add(FORMAT(L"<session xmlns=\"http://winscp.net/schema/session/1.0\" name=\"%s\" start=\"%s\">",
      XmlAttributeEscape(FSessionData->GetSessionName()).c_str(), XmlTimestamp().c_str()));
  }
  else if (!ALogging && FLogging)
  {
    if (FInGroup)
    {
      EndGroup();
    }
    Add(L"</session>");
    CloseLogFile();
    FLogging = false;
  }

}
//---------------------------------------------------------------------------
void __fastcall TActionLog::CloseLogFile()
{
  if (FFile != NULL)
  {
    fclose((FILE *)FFile);
    FFile = NULL;
  }
  FCurrentLogFileName = L"";
  FCurrentFileName = L"";
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::OpenLogFile()
{
  try
  {
    assert(FFile == NULL);
    assert(FConfiguration != NULL);
    FCurrentLogFileName = FConfiguration->GetActionsLogFileName();
    FFile = OpenFile(FCurrentLogFileName, FSessionData, false, FCurrentFileName);
  }
  catch (const std::exception & E)
  {
    // We failed logging to file, turn it off and notify user.
    FCurrentLogFileName = L"";
    FCurrentFileName = L"";
    FConfiguration->SetLogActions(false);
    try
    {
      throw ExtException(FMTLOAD(LOG_GEN_ERROR), &E);
    }
    catch (const std::exception & E)
    {
      FUI->HandleExtendedException(&E);
    }
  }
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::AddPendingAction(TSessionActionRecord * Action)
{
  FPendingActions->Add(Action);
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::RecordPendingActions()
{
    while ((FPendingActions->GetCount() > 0) &&
         static_cast<TSessionActionRecord *>(FPendingActions->GetItem(0))->Record())
    {
        FPendingActions->Delete(0);
    }
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::BeginGroup(std::wstring Name)
{
      assert(!FInGroup);
      FInGroup = true;
      assert(FIndent == L"  ");
      AddIndented(FORMAT(L"<group name=\"%s\" start=\"%s\">",
            XmlAttributeEscape(Name).c_str(), XmlTimestamp().c_str()));
      FIndent = L"    ";
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::EndGroup()
{
  assert(FInGroup);
  FInGroup = false;
  assert(FIndent == L"    ");
  FIndent = L"  ";
  AddIndented(L"</group>");
}
//---------------------------------------------------------------------------
void __fastcall TActionLog::SetEnabled(bool value)
{
  if (GetEnabled() != value)
  {
    FEnabled = value;
    ReflectSettings();
  }
}
