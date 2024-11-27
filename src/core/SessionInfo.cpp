
#include <vcl.h>
#pragma hdrstop

#include <stdio.h>
#include <lmcons.h>
#ifndef SECURITY_WIN32
#define SECURITY_WIN32
#endif
#include <sspi.h>
#include <secext.h>

#include <Common.h>
#include <nbutils.h>

#include "SessionInfo.h"
#include "Exceptions.h"
#include "TextsCore.h"
#include "CoreMain.h"
#include "Script.h"
#include <System.IOUtils.hpp>
#include <DateUtils.hpp>
#if defined(__BORLANDC__)
#pragma package(smart_init)
#endif // defined(__BORLANDC__)

static UnicodeString DoXmlEscape(const UnicodeString & AStr, bool NewLine)
{
  UnicodeString Str = AStr;
  for (int32_t Index = 1; Index <= Str.Length(); ++Index)
  {
    UnicodeString Repl;
    wchar_t Ch = Str[Index];
    switch (Ch)
    {
      case L'\x00': // \0 Is not valid in XML anyway
      case L'\x01':
      case L'\x02':
      case L'\x03':
      case L'\x04':
      case L'\x05':
      case L'\x06':
      case L'\x07':
      case L'\x08':
      // \n is handled below
      case L'\x0B':
      case L'\x0C':
      // \r is handled below
      case L'\x0E':
      case L'\x0F':
      case L'\x10':
      case L'\x11':
      case L'\x12':
      case L'\x13':
      case L'\x14':
      case L'\x15':
      case L'\x16':
      case L'\x17':
      case L'\x18':
      case L'\x19':
      case L'\x1A':
      case L'\x1B':
      case L'\x1C':
      case L'\x1D':
      case L'\x1E':
      case L'\x1F':
        Repl = L"#x" + ByteToHex(static_cast<uint8_t>(Ch)) + L";";
        break;

      case L'\xFFFE':
      case L'\xFFFF':
        Repl = L"#x" + CharToHex(Ch) + L";";
        break;

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
        Str.Delete(Index, 1);
        --Index;
        break;
    }

    if (!Repl.IsEmpty())
    {
      Str[Index] = L'&';
      Str.Insert(Repl, Index + 1);
      Index += Repl.Length();
    }
  }
  return Str;
}

static UnicodeString XmlEscape(const UnicodeString & Str)
{
  return DoXmlEscape(Str, false);
}

static UnicodeString XmlAttributeEscape(const UnicodeString & Str)
{
  return DoXmlEscape(Str, true);
}


// #pragma warn -inl
class TSessionActionRecord : public TObject
{
public:
  static bool classof(const TObject * Obj) { return Obj->is(OBJECT_CLASS_TSessionActionRecord); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_TSessionActionRecord) || TObject::is(Kind); }
public:
  explicit TSessionActionRecord(TActionLog * Log, TLogAction Action) :
    TObject(OBJECT_CLASS_TSessionActionRecord),
    FLog(Log),
    FAction(Action),
    FState(Opened),
    FRecursive(false),
    FErrorMessages(nullptr),
    FNames(std::make_unique<TStringList>()),
    FValues(std::make_unique<TStringList>()),
    FFileList(nullptr),
    FFile(nullptr)
  {
    FLog->AddPendingAction(this);
  }

  ~TSessionActionRecord() override
  {
#if defined(__BORLANDC__)
    delete FErrorMessages;
    delete FNames;
    delete FValues;
    delete FFileList;
    delete FFile;
#endif
    SAFE_DESTROY(FErrorMessages);
//    SAFE_DESTROY(FNames);
//    SAFE_DESTROY(FValues);
    SAFE_DESTROY(FFileList);
    SAFE_DESTROY(FFile);
  }

  void Restart()
  {
    FState = Opened;
    FRecursive = false;
#if defined(__BORLANDC__)
    delete FErrorMessages;
    FErrorMessages = nullptr;
    delete FFileList;
    FFileList = nullptr;
    delete FFile;
    FFile = nullptr;
#endif // defined(__BORLANDC__)
    SAFE_DESTROY(FErrorMessages);
    SAFE_DESTROY(FFileList);
    SAFE_DESTROY(FFile);
    FNames->Clear();
    FValues->Clear();
  }

  bool Record()
  {
    bool Result = (FState != Opened);
#if defined(__BORLANDC__)
    if (Result)
    {
      if (FLog->FLogging && (FState != Cancelled))
      {
        const wchar_t *Name = ActionName();
        UnicodeString Attrs;
        if (FRecursive)
        {
          Attrs = L" recursive=\"true\"";
        }
        FLog->AddIndented(FORMAT("<%s%s>", Name,  Attrs));
        for (int32_t Index = 0; Index < FNames->GetCount(); ++Index)
        {
          UnicodeString Value = FValues->GetString(Index);
          if (Value.IsEmpty())
          {
            FLog->AddIndented(FORMAT("  <%s />", FNames->GetString(Index)));
          }
          else
          {
            FLog->AddIndented(FORMAT("  <%s value=\"%s\" />",
                FNames->GetString(Index), XmlAttributeEscape(Value)));
          }
        }
        if (FFileList != nullptr)
        {
          FLog->AddIndented(L"  <files>");
          for (int32_t Index = 0; Index < FFileList->GetCount(); ++Index)
          {
            TRemoteFile * File = FFileList->GetFile(Index);
            RecordFile(L"    ", File, true);
          }
          FLog->AddIndented(L"  </files>");
        }
        if (FFile != nullptr)
        {
          RecordFile(L"  ", FFile, false);
        }
        if (FState == RolledBack)
        {
          if (FErrorMessages != nullptr)
          {
            FLog->AddIndented(L"  <result success=\"false\">");
            FLog->AddMessages(L"    ", FErrorMessages);
            FLog->AddIndented(L"  </result>");
          }
          else
          {
            FLog->AddIndented(L"  <result success=\"false\" />");
          }
        }
        else
        {
          FLog->AddIndented(L"  <result success=\"true\" />");
        }
        FLog->AddIndented(FORMAT("</%s>", Name));
      }
      delete this;
    }
#endif // defined(__BORLANDC__)
    return Result;
  }

  void Commit()
  {
    Close(Committed);
  }

  void Rollback(Exception * E)
  {
    DebugAssert(FErrorMessages == nullptr);
    FErrorMessages = ExceptionToMoreMessages(E);
    Close(RolledBack);
  }

  void Cancel()
  {
    Close(Cancelled);
  }

  void SetFileName(const UnicodeString & AFileName)
  {
    Parameter(L"filename", AFileName);
  }

  void Destination(const UnicodeString & Destination)
  {
    Parameter(L"destination", Destination);
  }

  void Size(int64_t Size)
  {
    Parameter(L"size", Int64ToStr(Size));
  }

  void Rights(const TRights & Rights)
  {
    Parameter(L"permissions", Rights.GetText());
  }

  void Modification(const TDateTime & DateTime)
  {
    Parameter(L"modification", StandardTimestamp(DateTime));
  }

  void Recursive()
  {
    FRecursive = true;
  }

  void Command(const UnicodeString & Command)
  {
    Parameter(L"command", Command);
  }

  void AddOutput(const UnicodeString & Output, bool StdError)
  {
    const wchar_t * Name = (StdError ? L"erroroutput" : L"output");
    const int32_t Index = FNames->IndexOf(Name);
    if (Index >= 0)
    {
      FValues->SetString(Index, FValues->GetString(Index) + L"\r\n" + Output);
    }
    else
    {
      Parameter(Name, Output);
    }
  }

  void ExitCode(int ExitCode)
  {
    Parameter(L"exitcode", ::IntToStr(ExitCode));
  }

  void Checksum(const UnicodeString & Alg, const UnicodeString & Checksum)
  {
    Parameter(L"algorithm", Alg);
    Parameter(L"checksum", Checksum);
  }

  void Cwd(const UnicodeString & Path)
  {
    Parameter(L"cwd", Path);
  }

  void FileList(TRemoteFileList * FileList)
  {
    if (FFileList == nullptr)
    {
      FFileList = new TRemoteFileList();
    }
    FileList->DuplicateTo(FFileList);
  }

  void File(TRemoteFile * AFile)
  {
    if (FFile != nullptr)
    {
      SAFE_DESTROY(FFile);
    }
    FFile = AFile->Duplicate(true);
  }

  void SynchronizeChecklistItem(const TChecklistItem * Item)
  {
    UnicodeString Action;
    bool RecordLocal = false;
    bool RecordRemote = false;
    switch (Item->Action)
    {
      case TChecklistAction::saUploadNew:
        Action = "uploadnew";
        RecordLocal = true;
        break;
      case TChecklistAction::saDownloadNew:
        Action = "downloadnew";
        RecordRemote = true;
        break;
      case TChecklistAction::saUploadUpdate:
        Action = "uploadupdate";
        RecordLocal = true;
        RecordRemote = true;
        break;
      case TChecklistAction::saDownloadUpdate:
        Action = "downloadupdate";
        RecordLocal = true;
        RecordRemote = true;
        break;
      case TChecklistAction::saDeleteRemote:
        Action = "deleteremote";
        RecordRemote = true;
        break;
      case TChecklistAction::saDeleteLocal:
        Action = "deletelocal";
        RecordLocal = true;
        break;
      default:
        DebugFail();
        break;
    }

    Parameter(L"action", Action);

    if (RecordLocal)
    {
      UnicodeString FileName = CombinePaths(Item->Local.Directory, Item->Local.FileName);
#if defined(__BORLANDC__)
      SynchronizeChecklistItemFileInfo(FileName, Item->IsDirectory, Item->Local);
#endif // defined(__BORLANDC__)
    }
    if (RecordRemote)
    {
      UnicodeString FileName = base::UnixCombinePaths(Item->Remote.Directory, Item->Remote.FileName);
#if defined(__BORLANDC__)
      SynchronizeChecklistItemFileInfo(FileName, Item->IsDirectory, Item->Remote);
#endif // defined(__BORLANDC__)
    }
  }


protected:
  enum TState { Opened, Committed, RolledBack, Cancelled };

  inline void Close(TState State)
  {
    DebugAssert(FState == Opened);
    FState = State;
    FLog->RecordPendingActions();
  }

  const wchar_t * ActionName() const
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
      case laCp: return L"cp";
      case laCall: return L"call";
      case laLs: return L"ls";
      case laStat: return L"stat";
      case laChecksum: return L"checksum";
      case laCwd: return L"cwd";
      case laDifference: return L"difference";
      DebugFail();
      return L"";
    }
  }

  void Parameter(const UnicodeString & Name, const UnicodeString & Value = "")
  {
    FNames->Add(Name);
    FValues->Add(Value);
  }

#if defined(__BORLANDC__)
  void RecordFile(const UnicodeString & AIndent, TRemoteFile * AFile, bool IncludeFileName)
  {
    FLog->AddIndented(AIndent + L"<file>");
    FLog->AddIndented(AIndent + FORMAT(L"  <filename value=\"%s\" />", (XmlAttributeEscape(AFile->FileName))));
    FLog->AddIndented(AIndent + FORMAT(L"  <type value=\"%s\" />", (XmlAttributeEscape(towupper(AFile->Type)))));
    if (!AFile->IsDirectory)
    {
      FLog->AddIndented(AIndent + FORMAT(L"  <size value=\"%s\" />", (IntToStr(AFile->Size))));
    }
    if (AFile->ModificationFmt != mfNone)
    {
      FLog->AddIndented(AIndent + FORMAT(L"  <modification value=\"%s\" />", (StandardTimestamp(AFile->Modification))));
    }
    if (!AFile->Rights->Unknown)
    {
      FLog->AddIndented(AIndent + FORMAT(L"  <permissions value=\"%s\" />", (XmlAttributeEscape(AFile->Rights->Text))));
    }
    if (AFile->Owner.IsSet)
    {
      FLog->AddIndented(AIndent + FORMAT(L"  <owner value=\"%s\" />", (XmlAttributeEscape(AFile->Owner.DisplayText))));
    }
    if (AFile->Group.IsSet)
    {
      FLog->AddIndented(AIndent + FORMAT(L"  <group value=\"%s\" />", (XmlAttributeEscape(AFile->Group.DisplayText))));
    }
    FLog->AddIndented(AIndent + L"</file>");
  }

  void SynchronizeChecklistItemFileInfo(
    const UnicodeString & AFileName, bool IsDirectory, const TChecklistItem::TFileInfo FileInfo)
  {
    Parameter(L"type", (IsDirectory ? L'D' : L'-'));
    FileName(AFileName);
    if (!IsDirectory)
    {
      Parameter(L"size", IntToStr(FileInfo.Size));
    }
    if (FileInfo.ModificationFmt != mfNone)
    {
      Modification(FileInfo.Modification);
    }
  }
#endif // defined(__BORLANDC__)

private:
  TActionLog * FLog{nullptr};
  TLogAction FAction{};
  TState FState{};
  bool FRecursive{false};
  TStrings * FErrorMessages{nullptr};
  std::unique_ptr<TStrings> FNames{nullptr};
  std::unique_ptr<TStrings> FValues{nullptr};
  TRemoteFileList * FFileList{nullptr};
  TRemoteFile * FFile{nullptr};
};
// #pragma warn .inl


TSessionAction::TSessionAction(TActionLog * Log, TLogAction Action) noexcept
{
  FCancelled = false;
  if (Log->FLogging)
  {
    FRecord = new TSessionActionRecord(Log, Action);
  }
  else
  {
    FRecord = nullptr;
  }
}

TSessionAction::~TSessionAction() noexcept
{
  try { if (FRecord != nullptr)
  {
    TSessionActionRecord * Record = FRecord;
    FRecord = nullptr;
    Record->Commit();
  } } catch(const std::exception &) { DEBUG_PRINTF("Error"); }
}

void TSessionAction::Restart()
{
  if (FRecord != nullptr)
  {
    FRecord->Restart();
  }
}

void TSessionAction::Rollback(Exception * E)
{
  if (FRecord != nullptr)
  {
    TSessionActionRecord * Record = FRecord;
    FRecord = nullptr;
    Record->Rollback(E);
  }
}

void TSessionAction::Cancel()
{
  if (FRecord != nullptr)
  {
    TSessionActionRecord * Record = FRecord;
    FRecord = nullptr;
    FCancelled = true;
    Record->Cancel();
  }
}

bool TSessionAction::IsValid() const
{
  return !FCancelled;
}


TFileSessionAction::TFileSessionAction(TActionLog * Log, TLogAction Action) noexcept :
  TSessionAction(Log, Action)
{
}

TFileSessionAction::TFileSessionAction(
  TActionLog * Log, TLogAction Action, const UnicodeString & AFileName) noexcept :
  TSessionAction(Log, Action)
{
  SetFileName(AFileName);
}

void TFileSessionAction::SetFileName(const UnicodeString & AFileName)
{
  if (FRecord != nullptr)
  {
    FRecord->SetFileName(AFileName);
  }
}


TFileLocationSessionAction::TFileLocationSessionAction(
  TActionLog * Log, TLogAction Action) noexcept :
  TFileSessionAction(Log, Action)
{
}

TFileLocationSessionAction::TFileLocationSessionAction(
  TActionLog * Log, TLogAction Action, const UnicodeString & AFileName) noexcept :
  TFileSessionAction(Log, Action, AFileName)
{
}

void TFileLocationSessionAction::Destination(const UnicodeString & Destination)
{
  if (FRecord != nullptr)
  {
    FRecord->Destination(Destination);
  }
}


TTransferSessionAction::TTransferSessionAction(TActionLog * Log, TLogAction Action) :
  TFileLocationSessionAction(Log, Action)
{
}

void TTransferSessionAction::Size(int64_t Size)
{
  if (FRecord != nullptr)
  {
    FRecord->Size(Size);
  }
}

TUploadSessionAction::TUploadSessionAction(TActionLog * Log) noexcept :
  TTransferSessionAction(Log, laUpload)
{
}


TDownloadSessionAction::TDownloadSessionAction(TActionLog * Log) noexcept :
  TTransferSessionAction(Log, laDownload)
{
}


TChmodSessionAction::TChmodSessionAction(
  TActionLog * Log, const UnicodeString & AFileName) noexcept :
  TFileSessionAction(Log, laChmod, AFileName)
{
}

void TChmodSessionAction::Recursive()
{
  if (FRecord != nullptr)
  {
    FRecord->Recursive();
  }
}

TChmodSessionAction::TChmodSessionAction(
  TActionLog * Log, const UnicodeString & AFileName, const TRights & ARights) noexcept :
  TFileSessionAction(Log, laChmod, AFileName)
{
  Rights(ARights);
}

void TChmodSessionAction::Rights(const TRights & Rights)
{
  if (FRecord != nullptr)
  {
    FRecord->Rights(Rights);
  }
}

TTouchSessionAction::TTouchSessionAction(
  TActionLog * Log, const UnicodeString & AFileName, const TDateTime & Modification) noexcept :
  TFileSessionAction(Log, laTouch, AFileName)
{
  if (FRecord != nullptr)
  {
    FRecord->Modification(Modification);
  }
}

TMkdirSessionAction::TMkdirSessionAction(
  TActionLog * Log, const UnicodeString & AFileName) noexcept :
  TFileSessionAction(Log, laMkdir, AFileName)
{
}

TRmSessionAction::TRmSessionAction(
  TActionLog * Log, const UnicodeString & AFileName) noexcept :
  TFileSessionAction(Log, laRm, AFileName)
{
}

void TRmSessionAction::Recursive()
{
  if (FRecord != nullptr)
  {
    FRecord->Recursive();
  }
}

TMvSessionAction::TMvSessionAction(TActionLog * Log,
  const UnicodeString & AFileName, const UnicodeString & ADestination) noexcept :
  TFileLocationSessionAction(Log, laMv, AFileName)
{
  Destination(ADestination);
}

TCpSessionAction::TCpSessionAction(TActionLog * Log,
  const UnicodeString & AFileName, const UnicodeString & ADestination) noexcept :
  TFileLocationSessionAction(Log, laCp, AFileName)
{
  Destination(ADestination);
}

TCallSessionAction::TCallSessionAction(TActionLog * Log,
  const UnicodeString & ACommand, const UnicodeString & ADestination) noexcept :
  TSessionAction(Log, laCall)
{
  if (FRecord != nullptr)
  {
    FRecord->Command(ACommand);
    FRecord->Destination(ADestination);
  }
}

void TCallSessionAction::AddOutput(const UnicodeString & Output, bool StdError)
{
  if (FRecord != nullptr)
  {
    FRecord->AddOutput(Output, StdError);
  }
}

void TCallSessionAction::ExitCode(int32_t ExitCode)
{
  if (FRecord != nullptr)
  {
    FRecord->ExitCode(ExitCode);
  }
}

TLsSessionAction::TLsSessionAction(TActionLog * Log,
  const UnicodeString & Destination) noexcept :
  TSessionAction(Log, laLs)
{
  if (FRecord != nullptr)
  {
    FRecord->Destination(Destination);
  }
}

void TLsSessionAction::FileList(TRemoteFileList * FileList)
{
  if (FRecord != nullptr)
  {
    FRecord->FileList(FileList);
  }
}


TStatSessionAction::TStatSessionAction(TActionLog * Log, const UnicodeString & AFileName) noexcept :
  TFileSessionAction(Log, laStat, AFileName)
{
}

void TStatSessionAction::File(TRemoteFile * AFile)
{
  if (FRecord != nullptr)
  {
    FRecord->File(AFile);
  }
}


TChecksumSessionAction::TChecksumSessionAction(TActionLog * Log) noexcept :
  TFileSessionAction(Log, laChecksum)
{
}

void TChecksumSessionAction::Checksum(const UnicodeString & Alg, const UnicodeString & Checksum)
{
  if (FRecord != nullptr)
  {
    FRecord->Checksum(Alg, Checksum);
  }
}


TCwdSessionAction::TCwdSessionAction(TActionLog * Log, const UnicodeString & Path) noexcept :
  TSessionAction(Log, laCwd)
{
  if (FRecord != nullptr)
  {
    FRecord->Cwd(Path);
  }
}


TDifferenceSessionAction::TDifferenceSessionAction(TActionLog * Log, const TChecklistItem * Item) noexcept :
  TSessionAction(Log, laDifference)
{
  if (FRecord != nullptr)
  {
    FRecord->SynchronizeChecklistItem(Item);
  }
}


TSessionInfo::TSessionInfo() noexcept
{
  LoginTime = Now();
  CertificateVerifiedManually = false;
}

TFileSystemInfo::TFileSystemInfo() noexcept
{
  // memset(&IsCapable, false, sizeof(IsCapable));
  nb::ClearArray(IsCapable);
}


static FILE * OpenLogFile(const UnicodeString & LogFileName, const TDateTime & Started, TSessionData * SessionData, bool Append, UnicodeString & ANewFileName)
{
  // FILE * Result;
  const UnicodeString NewFileName = StripPathQuotes(GetExpandedLogFileName(LogFileName, Started, SessionData));
  FILE * Result = _wfsopen(ApiPath(NewFileName).c_str(), Append ? L"ab" : L"wb", SH_DENYWR);
  if (Result != nullptr)
  {
    constexpr size_t BUFSIZE = 4 * 1024;
    setvbuf(Result, nullptr, _IONBF, BUFSIZE);
    ANewFileName = NewFileName;
  }
  else
  {
    throw ECRTExtException(FMTLOAD(LOG_OPENERROR, NewFileName));
  }
  return Result;
}


constexpr wchar_t * LogLineMarks = L"<>!.*";
TSessionLog::TSessionLog(gsl::not_null<TSessionUI *> UI, const TDateTime & Started, gsl::not_null<TSessionData *> SessionData,
  TConfiguration * Configuration) noexcept :
  FConfiguration(Configuration),
  FParent(nullptr),
  FLogging(false),
  FLogger(nullptr),
  FCurrentFileSize(0),
  FUI(UI),
  FSessionData(SessionData),
  FStarted(Started),
  FClosed(false)
{
  // DEBUG_PRINTF("begin");
#if defined(__BORLANDC__)
  FCriticalSection = new TCriticalSection;
  FLogging = false;
  FConfiguration = Configuration;
  FParent = nullptr;
  FUI = UI;
  FSessionData = SessionData;
  FStarted = Started;
  FFile = nullptr;
  FCurrentLogFileName = L"";
  FCurrentFileName = L"";
  FClosed = false;
#endif
  // DEBUG_PRINTF("begin");
}

TSessionLog::~TSessionLog() noexcept
{ try {
  // DEBUG_PRINTF("end");
  FClosed = true;
  if (FLogger)
    FLogger->Close();
  ReflectSettings();
  DebugAssert(FLogger == nullptr);
#if defined(__BORLANDC__)
  delete FCriticalSection;
#endif // defined(__BORLANDC__)
  // DEBUG_PRINTF("end");
  } catch(const std::exception &) { DEBUG_PRINTF("Error"); }
}

void TSessionLog::SetParent(gsl::not_null<TSessionLog *> AParent, const UnicodeString & AName)
{
  FParent = AParent;
  FName = AName;
}

void TSessionLog::DoAddToParent(TLogLineType Type, const UnicodeString & ALine)
{
  DebugAssert(FParent != nullptr);
  FParent->Add(Type, ALine);
}

void TSessionLog::DoAddToSelf(TLogLineType Type, const UnicodeString & ALine)
{
  if (LogToFile()) { try
  {
    if (FLogger == nullptr)
    {
      OpenLogFile();
    }

    if (FLogger != nullptr)
    {
      const UTF8String UtfLine = UTF8String(UnicodeString(1, LogLineMarks[Type]) + " " + TrimRight(ALine)); // without timestamp
#if defined(__BORLANDC__)
      const UnicodeString Timestamp = FormatDateTime(L" yyyy-mm-dd hh:nn:ss.zzz ", Now());
      const UTF8String UtfLine = UTF8String(UnicodeString(1, LogLineMarks[Type]) + Timestamp + TrimRight(ALine)); // + "\r\n";
      for (int32_t Index = 1; Index <= UtfLine.Length(); Index++)
      {
        if ((UtfLine[Index] == '\n') &&
            ((Index == 1) || (UtfLine[Index - 1] != '\r')))
        {
          UtfLine.Insert(L'\r', Index);
        }
      }
#endif // defined(__BORLANDC__)
      const int32_t ToWrite = UtfLine.Length();
      CheckSize(ToWrite);
      FCurrentFileSize += FLogger->Write(UtfLine.c_str(), ToWrite);
      // FLogger->GetLogStream("", __LINE__, __func__, tinylog::Utils::LEVEL_TRACE) << UtfLine;
      // TINYLOG_TRACE(g_tinylog) << repr("FAcquired: %d", FAcquired);
    }}
    catch (...)
    {
      // TODO: log error
      DEBUG_PRINTF("TSessionLog::DoAddToSelf: error");
    }
  }
}

UnicodeString TSessionLog::LogPartFileName(const UnicodeString & BaseName, int32_t Index)
{
  UnicodeString Result;
  if (Index >= 1)
  {
    Result = FORMAT("%s.%d", BaseName, Index);
  }
  else
  {
    Result = BaseName;
  }
  return Result;
}

void TSessionLog::CheckSize(int64_t Addition)
{
  const int64_t MaxSize = FConfiguration->GetLogMaxSize();
  if ((MaxSize > 0) && (FCurrentFileSize + Addition >= MaxSize))
  {
    // Before we close it
    const UnicodeString BaseName = FCurrentFileName;
    CloseLogFile();
    FCurrentFileSize = 0;

    int32_t Index = 0;

    while (base::FileExists(LogPartFileName(BaseName, Index + 1)))
    {
      Index++;
    }

    const int32_t MaxCount = FConfiguration->GetLogMaxCount();

    do
    {
      UnicodeString LogPart = LogPartFileName(BaseName, Index);
      if ((MaxCount > 0) && (Index >= MaxCount))
      {
        DeleteFileChecked(LogPart);
      }
      else
      {
        THROWOSIFFALSE(base::RenameFile(LogPart, LogPartFileName(BaseName, Index + 1)));
      }
      Index--;
    }
    while (Index >= 0);

    OpenLogFile();
  }
}

void TSessionLog::DoAdd(TLogLineType AType, const UnicodeString & ALine,
  // void (__closure *f)(TLogLineType Type, const UnicodeString & Line))
  TDoAddLogEvent && Event)
{
  UnicodeString Prefix;

  if (!GetName().IsEmpty())
  {
    Prefix = L"[" + GetName() + L"] ";
  }

  UnicodeString Line = ALine;
  while (!Line.IsEmpty())
  {
    Event(AType, Prefix + CutToChar(Line, L'\n', false));
  }
}

void TSessionLog::Add(TLogLineType Type, const UnicodeString & ALine)
{
  DebugAssert(FConfiguration);
  if (GetLogging())
  {
    try
    {
      if (FParent != nullptr)
      {
        DoAdd(Type, ALine, nb::bind(&TSessionLog::DoAddToParent, this));
      }
      else
      {
        const TGuard Guard(FCriticalSection);

        DoAdd(Type, ALine, nb::bind(&TSessionLog::DoAddToSelf, this));
      }
    }
    catch(Exception & E)
    {
      // We failed logging, turn it off and notify user.
      FConfiguration->SetLogging(false);
      try
      {
        throw ExtException(&E, MainInstructions(LoadStr(LOG_GEN_ERROR)));
      }
      catch(Exception & E2)
      {
        AddException(&E2);
        FUI->HandleExtendedException(&E2);
      }
    }
  }
}

void TSessionLog::AddException(Exception * E)
{
  if (E != nullptr)
  {
    Add(llException, ExceptionLogString(E));
  }
}

void TSessionLog::ReflectSettings()
{
  const TGuard Guard(FCriticalSection);

  FLogging =
    !FClosed &&
    ((FParent != nullptr) || FConfiguration->GetLogging());

  // if logging to file was turned off or log file was changed -> close current log file
  if ((FLogger != nullptr) &&
      (!LogToFile() || (FCurrentLogFileName != FConfiguration->GetLogFileName())))
  {
    CloseLogFile();
  }

  if (FLogger != nullptr)
  {
    CheckSize(0);
  }

}

bool TSessionLog::LogToFileProtected() const
{
  return GetLogging() && FConfiguration->GetLogToFile() && (FParent == nullptr);
}

void TSessionLog::CloseLogFile()
{
  if (FLogger != nullptr)
  {
#if defined(__BORLANDC__)
    fclose((FILE *)FFile);
    FFile = nullptr;
#endif // defined(__BORLANDC__)
    FLogger.reset();
  }
  FCurrentLogFileName.Clear();
  FCurrentFileName.Clear();
}

void TSessionLog::OpenLogFile()
{
  DebugAssert(FConfiguration != nullptr);
  if (!FConfiguration)
    return;
  try
  {
    DebugAssert(FLogger == nullptr);
    DebugAssert(FConfiguration != nullptr);
    FCurrentLogFileName = FConfiguration->GetLogFileName();
    FILE * file = ::OpenLogFile(FCurrentLogFileName, FStarted, FSessionData, FConfiguration->GetLogFileAppend(), FCurrentFileName);
    FLogger = std::make_unique<tinylog::TinyLog>();
    FLogger->file(file);
    TSearchRec SearchRec;
    if (FileSearchRec(FCurrentFileName, SearchRec))
    {
      FCurrentFileSize = SearchRec.Size;
    }
    else
    {
      FCurrentFileSize = 0;
    }
  }
  catch(Exception & E)
  {
    // We failed logging to file, turn it off and notify user.
    FCurrentLogFileName.Clear();
    FCurrentFileName.Clear();
    FConfiguration->SetLogFileName(UnicodeString());
    try
    {
      throw ExtException(&E, MainInstructions(LoadStr(LOG_GEN_ERROR)));
    }
    catch(Exception & E2)
    {
      AddException(&E2);
      // not to deadlock with TSessionLog::ReflectSettings invoked by FConfiguration->LogFileName setter above
      const TUnguard Unguard(FCriticalSection);
      FUI->HandleExtendedException(&E2);
    }
  }

  // in case we are appending and the existing log file is already too large
  if (FLogger != nullptr)
  {
    CheckSize(0);
  }
}

void TSessionLog::AddSystemInfo()
{
  AddStartupInfo(true);
}

void TSessionLog::AddStartupInfo()
{
  AddStartupInfo(false);
}

void TSessionLog::AddStartupInfo(bool System)
{
  TSessionData * Data = (System ? nullptr : FSessionData);
  if (GetLogging())
  {
    if (FParent != nullptr)
    {
      // do not add session info for secondary session
      // (this should better be handled in the TSecondaryTerminal)
    }
    else
    {
      DoAddStartupInfo(Data);
    }
  }
}

UnicodeString TSessionLog::LogSensitive(const UnicodeString & Str)
{
  if (FConfiguration->GetLogSensitive() && !Str.IsEmpty())
  {
    return NormalizeString(Str);
  }
  return BooleanToEngStr(!Str.IsEmpty());
}

UnicodeString TSessionLog::GetCmdLineLog(TConfiguration * AConfiguration)
{
  TODO("GetCmdLine()");
  UnicodeString Result; // = CmdLine;

  if (!AConfiguration->GetLogSensitive())
  {
#if defined(__BORLANDC__)
    TManagementScript Script(StoredSessions, false);
    Script.MaskPasswordInCommandLine(Result, true);
#endif // defined(__BORLANDC__)
  }

  return Result;
}

template <typename T>
UnicodeString EnumName(T Value, const UnicodeString & ANames)
{
  int32_t N = nb::ToInt32(Value);
  UnicodeString Names = ANames;
  do
  {
    UnicodeString Name = CutToChar(Names, L';', true);
    if (N == 0)
    {
      return Name;
    }
    --N;
  }
  while ((N >= 0) && !Names.IsEmpty());

  return L"(unknown)";
}
//#define ADSTR(S) AddLogEntry(S)
//#define ADF(S, F) ADSTR(FORMAT(S, F));
#define ADSTR(S) DoAdd(llMessage, S, nb::bind(&TSessionLog::DoAddToSelf, this))
#define ADF(S, ...) DoAdd(llMessage, FORMAT(S, __VA_ARGS__), nb::bind(&TSessionLog::DoAddToSelf, this))

void TSessionLog::DoAddStartupInfo(TAddLogEntryEvent && OnAddLogEntry, TConfiguration * AConfiguration, bool DoNotMaskPasswords)
{
  ADSTR(GetEnvironmentInfo());
  std::unique_ptr<THierarchicalStorage> Storage(FConfiguration->CreateConfigStorage());
  try__finally
  {
    ADF(L"Configuration: %s", Storage->Source());
  }
  __finally__removed
  {
#if defined(__BORLANDC__)
    delete Storage;
#endif // defined(__BORLANDC__)
  } end_try__finally

  wchar_t UserName[UNLEN + 1];
  ULONG UserNameSize = LENOF(UserName);
  if (DebugAlwaysFalse(!::GetUserNameEx(NameSamCompatible, UserName, &UserNameSize)))
  {
    wcscpy_s(UserName, _countof(UserName), L"<Failed to retrieve username>");
  }
  UnicodeString LogStr;
  if (AConfiguration->LogProtocol <= -1)
  {
    LogStr = L"Reduced";
  }
  else if (AConfiguration->LogProtocol <= 0)
  {
    LogStr = L"Normal";
  }
  else if (AConfiguration->LogProtocol == 1)
  {
    LogStr = L"Debug 1";
  }
  else if (AConfiguration->LogProtocol >= 2)
  {
    LogStr = L"Debug 2";
  }
  if (AConfiguration->FLogSensitive)
  {
    LogStr += L", Logging passwords";
  }
  if (AConfiguration->FLogMaxSize > 0)
  {
    LogStr += FORMAT(L", Rotating after: %s", SizeToStr(AConfiguration->FLogMaxSize));
    if (AConfiguration->FLogMaxCount > 0)
    {
      LogStr += FORMAT(L", Keeping at most %d logs", AConfiguration->FLogMaxCount);
    }
  }
  ADF(L"Log level: %s", LogStr);
  ADF(L"Local account: %s", UserName);
  ADF(L"Working directory: %s", GetCurrentDir());
  ADF(L"Process ID: %d", nb::ToInt32(GetCurrentProcessId()));
  ADF(L"Ancestor processes: %s", GetAncestorProcessNames());
  // This logs even passwords, contrary to a session log.
  // GetCmdLineLog requires master password, but we do not know it yet atm.
  UnicodeString ACmdLine;
  if (DoNotMaskPasswords)
  {
    ACmdLine = ""; // CmdLine;
  }
  else
  {
    ACmdLine = GetCmdLineLog(AConfiguration);
  }
  ADF(L"Command-line: %s", ACmdLine);
  if (AConfiguration->ActualLogProtocol >= 1)
  {
    GetGlobalOptions()->LogOptions(std::move(OnAddLogEntry));
  }
  ADF(L"Time zone: %s", GetTimeZoneLogString());
  if (!AdjustClockForDSTEnabled())
  {
    ADSTR(L"Warning: System option \"Automatically adjust clock for Daylight Saving Time\" is disabled, timestamps will not be represented correctly");
  }
}

#undef ADSTR
#if defined(__BORLANDC__)
#define ADSTR(S) DoAdd(llMessage, S, DoAddToSelf);
#endif // defined(__BORLANDC__)
#define ADSTR(S) DoAdd(llMessage, S, nb::bind(&TSessionLog::DoAddToSelf, this))

void TSessionLog::DoAddStartupInfoEntry(const UnicodeString & S)
{
  ADSTR(S);
}

void TSessionLog::DoAddStartupInfo(TSessionData * Data)
{
  if (Data == nullptr)
  {
    AddSeparator();
    DoAddStartupInfo(nb::bind(&TSessionLog::DoAddStartupInfoEntry, this), FConfiguration, false);
    ADF(L"Login time: %s", FormatDateTime(L"dddddd tt", Now()));
    AddSeparator();
  }
  else
  {
    ADF(L"Session name: %s (%s)", Data->GetSessionName(), Data->GetSourceName());
    UnicodeString AddressFamily;
    if (Data->AddressFamily != afAuto)
    {
      AddressFamily = FORMAT(L"%s, ", Data->AddressFamily() == afIPv4 ? L"IPv4" : L"IPv6");
    }
    UnicodeString HostName = Data->HostNameExpanded;
    if (!Data->GetHostNameSource().IsEmpty())
    {
      HostName = FORMAT(L"%s [%s]", HostName, Data->GetHostNameSource());
    }
    ADF(L"Host name: %s (%sPort: %d)", HostName, AddressFamily, Data->PortNumber());
    UnicodeString UserName = Data->GetUserNameExpanded();
    if (!Data->GetUserNameSource().IsEmpty())
    {
      UserName = FORMAT(L"%s [%s]", UserName, Data->GetUserNameSource());
    }
    ADF(L"User name: %s (Password: %s, Key file: %s, Passphrase: %s)",
      UserName, LogSensitive(Data->GetPassword()),
       LogSensitive(Data->ResolvePublicKeyFile()), LogSensitive(Data->Passphrase));
    if (Data->GetUsesSsh())
    {
      ADF(L"Tunnel: %s", BooleanToEngStr(Data->FTunnel));
      if (Data->FTunnel)
      {
        ADF(L"Tunnel: Host name: %s (Port: %d)", Data->GetTunnelHostName(), Data->GetTunnelPortNumber());
        ADF(L"Tunnel: User name: %s (Password: %s, Key file: %s)",
           Data->FTunnelUserName,
           LogSensitive(Data->GetTunnelPassword()),
           LogSensitive(Data->FTunnelPublicKeyFile));
        ADF(L"Tunnel: Local port number: %d", Data->GetTunnelLocalPortNumber());
      }
    }
    ADF(L"Transfer Protocol: %s", Data->GetFSProtocolStr());
    if (Data->GetUsesSsh() || (Data->GetFSProtocol() == fsFTP))
    {
      UnicodeString PingType;
      int32_t PingInterval;
      if (Data->FSProtocol == fsFTP)
      {
        PingType = EnumName(Data->GetFtpPingType(), FtpPingTypeNames);
        PingInterval = Data->GetFtpPingInterval();
      }
      else
      {
        PingType = EnumName(Data->PingType(), PingTypeNames);
        PingInterval = Data->PingInterval;
      }
      ADF("Ping type: %s, Ping interval: %d sec; Timeout: %d sec",
        PingType, PingInterval, Data->GetTimeout());
      ADF("Disable Nagle: %s",
        BooleanToEngStr(Data->GetTcpNoDelay()));
    }
    ADF(L"Proxy: %s",
      (Data->FFtpProxyLogonType != 0) ?
        FORMAT(L"FTP proxy %d", Data->FFtpProxyLogonType) :
        EnumName(Data->FProxyMethod, ProxyMethodNames));
    if ((Data->FFtpProxyLogonType != 0) || (Data->FProxyMethod != ::pmNone))
    {
      ADF("ProxyHostName: %s (Port: %d); ProxyUsername: %s; Passwd: %s",
         Data->FProxyHost, Data->FProxyPort,
         Data->FProxyUsername, LogSensitive(Data->GetProxyPassword()));
      if (Data->FProxyMethod == pmTelnet)
      {
        ADF("Telnet command: %s", Data->FProxyTelnetCommand);
      }
      if (Data->FProxyMethod == pmCmd)
      {
        ADF("Local command: %s", Data->FProxyLocalCommand);
      }
    }
    if (Data->GetUsesSsh() || (Data->GetFSProtocol() == fsFTP) || (Data->FSProtocol == fsS3))
    {
      ADF(L"Send buffer: %d", Data->FSendBuf);
    }
    if (Data->GetUsesSsh() && !Data->FSourceAddress.IsEmpty())
    {
      ADF(L"Source address: %s", Data->FSourceAddress);
    }
    if (Data->GetUsesSsh())
    {
      ADF(L"Compression: %s", BooleanToEngStr(Data->FCompression));
      ADF(L"Bypass authentication: %s",
       BooleanToEngStr(Data->FSshNoUserAuth));
      ADF(L"Try agent: %s; Agent forwarding: %s; KI: %s; GSSAPI: %s",
        BooleanToEngStr(Data->FTryAgent), BooleanToEngStr(Data->FAgentFwd),
         BooleanToEngStr(Data->FAuthKI), BooleanToEngStr(Data->FAuthGSSAPI));
      if (Data->FAuthGSSAPI)
      {
        ADF(L"GSSAPI: KEX: %s; Forwarding: %s; Libs: %s; Custom: %s",
          BooleanToEngStr(Data->FAuthGSSAPIKEX), BooleanToEngStr(Data->FGSSAPIFwdTGT), Data->GetGssLibList(), Data->FGssLibCustom);
      }
      ADF("Ciphers: %s; Ssh2DES: %s",
        Data->GetCipherList(), BooleanToEngStr(Data->FSsh2DES));
      ADF("KEX: %s", Data->GetKexList());
      UnicodeString Bugs;
      for (int32_t Index = 0; Index < BUG_COUNT; ++Index)
      {
        AddToList(Bugs, EnumName(Data->GetBug(static_cast<TSshBug>(Index)), AutoSwitchNames), L",");
      }
      ADF("SSH Bugs: %s", Bugs);
      ADF("Simple channel: %s", BooleanToEngStr(Data->FSshSimple));
      ADF("Return code variable: %s; Lookup user groups: %s",
        Data->GetDetectReturnVar() ? UnicodeString(L"Autodetect") : Data->FReturnVar,
         EnumName(Data->FLookupUserGroups, AutoSwitchNames));
      ADF("Shell: %s", Data->GetShell().IsEmpty() ? UnicodeString(L"default") : Data->FShell);
      ADF("EOL: %s, UTF: %s", EnumName(Data->GetEOLType(), EOLTypeNames), EnumName(Data->GetNotUtf(), NotAutoSwitchNames)); // NotUtf duplicated in FTP branch
      ADF("Clear aliases: %s, Unset nat.vars: %s, Resolve symlinks: %s; Follow directory symlinks: %s",
        BooleanToEngStr(Data->FClearAliases), BooleanToEngStr(Data->FUnsetNationalVars),
         BooleanToEngStr(Data->FResolveSymlinks), BooleanToEngStr(Data->FFollowDirectorySymlinks));
      ADF("LS: %s, Ign LS warn: %s, Scp1 Comp: %s; Exit code 1 is error: %s",
         Data->FListingCommand,
         BooleanToEngStr(Data->FIgnoreLsWarnings),
         BooleanToEngStr(Data->FScp1Compatibility),
         BooleanToEngStr(Data->FExitCode1IsError));
    }
    if ((Data->GetFSProtocol() == fsSFTP) || (Data->GetFSProtocol() == fsSFTPonly))
    {
      UnicodeString Bugs;
      for (int32_t Index = 0; Index < SFTP_BUG_COUNT; ++Index)
      {
        AddToList(Bugs, EnumName(Data->GetSFTPBug(static_cast<TSftpBug>(Index)), AutoSwitchNames), L",");
      }
      ADF("SFTP Bugs: %s", Bugs);
      ADF("SFTP Server: %s", Data->FSftpServer.IsEmpty() ? UnicodeString(L"default") : Data->FSftpServer);
      if (Data->FSFTPRealPath != asAuto)
      {
        ADF(L"SFTP Real path: %s", EnumName(Data->FSFTPRealPath, AutoSwitchNames));
      }
      if (Data->UsePosixRename)
      {
        ADF(L"Use POSIX rename: %s", BooleanToEngStr(Data->UsePosixRename));
      }
    }
    bool FtpsOn = false;
    if (Data->GetFSProtocol() == fsFTP)
    {
      ADF("UTF: %s", EnumName(Data->GetNotUtf(), NotAutoSwitchNames)); // duplicated in UsesSsh branch
      UnicodeString Ftps;
      switch (Data->GetFtps())
      {
        case ftpsImplicit:
          Ftps = L"Implicit TLS/SSL";
          FtpsOn = true;
          break;

        case ftpsExplicitSsl:
          Ftps = L"Explicit SSL/TLS";
          FtpsOn = true;
          break;

        case ftpsExplicitTls:
          Ftps = L"Explicit TLS/SSL";
          FtpsOn = true;
          break;

        default:
          DebugAssert(Data->GetFtps() == ftpsNone);
          Ftps = L"None";
          break;
      }
      // kind of hidden option, so do not reveal it unless it is set
      if (Data->GetFtpTransferActiveImmediately() != asAuto)
      {
        ADF("Transfer active immediately: %s", EnumName(Data->GetFtpTransferActiveImmediately(), AutoSwitchNames));
      }
      ADF("FTPS: %s [Client certificate: %s]",
        Ftps, LogSensitive(Data->GetTlsCertificateFile()));
      ADF("FTP: Passive: %s [Force IP: %s]; MLSD: %s [List all: %s]; HOST: %s",
         BooleanToEngStr(Data->GetFtpPasvMode()),
         EnumName(Data->GetFtpForcePasvIp(), AutoSwitchNames),
         EnumName(Data->GetFtpUseMlsd(), AutoSwitchNames),
         EnumName(Data->GetFtpListAll(), AutoSwitchNames),
         EnumName(Data->GetFtpHost(), AutoSwitchNames));
      if (Data->FFtpWorkFromCwd != asAuto)
      {
        ADF(L"FTP: Relative paths: %s", EnumName(Data->FFtpWorkFromCwd, AutoSwitchNames));
      }
    }
    if (Data->GetFSProtocol() == fsWebDAV)
    {
      FtpsOn = (Data->GetFtps() != ftpsNone);
      ADF("HTTPS: %s [Client certificate: %s]",
        BooleanToEngStr(FtpsOn), LogSensitive(Data->GetTlsCertificateFile()));
      ADF(L"WebDAV: Tolerate non-encoded: %s", (BooleanToEngStr(Data->FWebDavLiberalEscaping)));
    }
    if (Data->GetFSProtocol() == fsS3)
    {
      FtpsOn = (Data->GetFtps() != ftpsNone);
      ADF(L"HTTPS: %s", BooleanToEngStr(FtpsOn));
      ADF(L"S3: URL Style: %s", EnumName(Data->FS3UrlStyle, L"Virtual Host;Path"));
      if (!Data->FS3DefaultRegion.IsEmpty())
      {
        ADF(L"S3: Default region: %s", Data->FS3DefaultRegion);
      }
      if (!Data->FS3SessionToken.IsEmpty())
      {
        ADF(L"S3: Session token: %s", Data->FS3SessionToken);
      }
      if (!Data->FS3RoleArn.IsEmpty())
      {
        ADF(L"S3: Role ARN: %s (session name: %s)", Data->S3RoleArn, DefaultStr(Data->S3RoleSessionName, L"default"));
      }
      if (Data->S3CredentialsEnv)
      {
        ADF(L"S3: Credentials from AWS environment: %s", DefaultStr(Data->S3Profile, L"General"));
      }
    }
    if (FtpsOn)
    {
      if (Data->GetFSProtocol() == fsFTP)
      {
        ADF("Session reuse: %s", BooleanToEngStr(Data->FSslSessionReuse));
      }
      ADF("TLS/SSL versions: %s-%s", GetTlsVersionName(Data->FMinTlsVersion), GetTlsVersionName(Data->FMaxTlsVersion));
    }
    ADF("Local directory: %s, Remote directory: %s, Update: %s, Cache: %s",
       Data->LocalDirectory().IsEmpty() ? UnicodeString(L"default") : Data->LocalDirectory(),
       Data->GetRemoteDirectory().IsEmpty() ? UnicodeString(L"home") : Data->GetRemoteDirectory(),
       BooleanToEngStr(Data->GetUpdateDirectories()),
       BooleanToEngStr(Data->GetCacheDirectories()));
    ADF("Cache directory changes: %s, Permanent: %s",
       BooleanToEngStr(Data->GetCacheDirectoryChanges()),
       BooleanToEngStr(Data->GetPreserveDirectoryChanges()));
    ADF("Recycle bin: Delete to: %s, Overwritten to: %s, Bin path: %s",
       BooleanToEngStr(Data->GetDeleteToRecycleBin()),
       BooleanToEngStr(Data->GetOverwrittenToRecycleBin()),
       Data->GetRecycleBinPath());
    if (Data->TrimVMSVersions() || Data->VMSAllRevisions())
    {
      ADF(L"Trim VMS versions: %s; VMS all revisions: %s",
        BooleanToEngStr(Data->TrimVMSVersions), BooleanToEngStr(Data->VMSAllRevisions));
    }
    UnicodeString TimeInfo;
    if ((Data->GetFSProtocol() == fsSFTP) || (Data->GetFSProtocol() == fsSFTPonly) || (Data->GetFSProtocol() == fsSCPonly) || (Data->GetFSProtocol() == fsWebDAV))
    {
      AddToList(TimeInfo, FORMAT("DST mode: %s", EnumName(nb::ToInt(Data->GetDSTMode()), DSTModeNames)), L";");
    }
    if ((Data->GetFSProtocol() == fsSCPonly) || (Data->GetFSProtocol() == fsFTP))
    {
      const int32_t TimeDifferenceMin = TimeToMinutes(Data->GetTimeDifference());
      AddToList(TimeInfo, FORMAT("Timezone offset: %dh %dm", TimeDifferenceMin / MinutesPerHour, TimeDifferenceMin % MinutesPerHour), L";");
    }
    ADSTR(TimeInfo);

    if (Data->GetFSProtocol() == fsWebDAV)
    {
      ADF("Compression: %s",
        BooleanToEngStr(Data->GetCompression()));
    }

    if ((Data->FSProtocol == fsFTP) && !Data->FFtpPasvMode)
    {
      if (!FConfiguration->FExternalIpAddress.IsEmpty() || FConfiguration->HasLocalPortNumberLimits())
      {
        AddSeparator();
        ADF(L"FTP active mode interface: %s:%d-%d", DefaultStr(FConfiguration->FExternalIpAddress, L"<system address>"), FConfiguration->FLocalPortNumberMin, FConfiguration->FLocalPortNumberMax);
      }
    }
    AddSeparator();
  }
}

#undef ADF
#undef ADSTR

UnicodeString TSessionLog::GetSeparator()
{
  return L"--------------------------------------------------------------------------";
}

void TSessionLog::AddSeparator()
{
  Add(llMessage, GetSeparator());
}


TActionLog::TActionLog(TSessionUI * UI, const TDateTime & Started, TSessionData * SessionData,
  TConfiguration * Configuration) noexcept :
  FConfiguration(Configuration),
  FUI(UI),
  FSessionData(SessionData),
  FPendingActions(std::make_unique<TList>()),
  FIndent(L"  ")
{
  DebugAssert(UI != nullptr);
  DebugAssert(SessionData != nullptr);
  Init(UI, Started, SessionData, Configuration);
}

TActionLog::TActionLog(const TDateTime Started, TConfiguration * Configuration) noexcept
{
  Init(nullptr, Started, nullptr, Configuration);
  // not associated with session, so no need to waiting for anything
  ReflectSettings();
}

void TActionLog::Init(TSessionUI * UI, const TDateTime & Started, TSessionData * SessionData,
  TConfiguration * Configuration)
{
  // FCriticalSection = new TCriticalSection;
  FConfiguration = Configuration;
  FUI = UI;
  FSessionData = SessionData;
  FStarted = Started;
  FLogger = nullptr;
  FCurrentLogFileName.Clear();
  FCurrentFileName.Clear();
  FLogging = false;
  FClosed = false;
  FFailed = false;
//  FPendingActions = new TList();
  FIndent = L"  ";
  FInGroup = false;
  FEnabled = true;
}

TActionLog::~TActionLog() noexcept
{ try {
  DebugAssert(FPendingActions->GetCount() == 0);
#if defined(__BORLANDC__)
  delete FPendingActions;
#endif // defined(__BORLANDC__)
  FClosed = true;
  ReflectSettings();
  DebugAssert(FLogger == nullptr);
#if defined(__BORLANDC__)
  delete FCriticalSection;
#endif // defined(__BORLANDC__)
  } catch(const std::exception &) { DEBUG_PRINTF("Error"); }
}

void TActionLog::Add(const UnicodeString & Line)
{
  DebugAssert(FConfiguration);
  if (FLogging)
  {
    const TGuard Guard(FCriticalSection);
    if (FLogger == nullptr)
    {
      OpenLogFile();
    }

    if (FLogger != nullptr)
    {
      try
      {
        const UTF8String UtfLine = UTF8String(Line);
        int64_t Written =
          FLogger->Write(UtfLine.c_str(), UtfLine.Length());
        if (Written != nb::ToInt64(UtfLine.Length()))
        {
          throw ECRTExtException("");
        }
        #ifdef _DEBUG
        #endif
        Written =
          FLogger->Write("\n", 1);
        if (Written != 1)
        {
          throw ECRTExtException("");
        }
      }
      catch(Exception & E)
      {
//        FCriticalSection.Release();

        // avoid endless loop when trying to close tags when closing log, when logging has failed
        if (!FFailed)
        {
          FFailed = true;
          // We failed logging, turn it off and notify user.
          FConfiguration->SetLogActions(false);
          if (FConfiguration->GetLogActionsRequired())
          {
            throw EFatal(&E, LoadStr(LOG_FATAL_ERROR));
          }
          else
          {
            try
            {
              throw ExtException(&E, LoadStr(LOG_GEN_ERROR));
            }
            catch(Exception & E2)
            {
              if (FUI != nullptr)
              {
                FUI->HandleExtendedException(&E2);
              }
            }
          }
        }
      }
    }
  }
}

void TActionLog::AddIndented(const UnicodeString & ALine)
{
  Add(FIndent + ALine);
}

void TActionLog::AddFailure(TStrings * Messages)
{
  AddIndented(L"<failure>");
  AddMessages(L"  ", Messages);
  AddIndented(L"</failure>");
}

void TActionLog::AddFailure(Exception * E)
{
  std::unique_ptr<TStrings> Messages(ExceptionToMoreMessages(E));
  if (Messages != nullptr)
  {
    try__finally
    {
      AddFailure(Messages.get());
    }
    __finally__removed
    {
#if defined(__BORLANDC__)
      delete Messages;
#endif // defined(__BORLANDC__)
    } end_try__finally
  }
}

void TActionLog::AddMessages(const UnicodeString & Indent, TStrings * Messages)
{
  for (int32_t Index = 0; Index < Messages->GetCount(); ++Index)
  {
    AddIndented(
      FORMAT((Indent + L"<message>%s</message>"), XmlEscape(Messages->GetString(Index))));
  }
}

void TActionLog::ReflectSettings()
{
  const TGuard Guard(FCriticalSection);

  const bool ALogging =
    !FClosed && FConfiguration->GetLogActions() && GetEnabled();

  if (ALogging && !FLogging)
  {
    FLogging = true;
#if defined(__BORLANDC__)
    Add(L"<?xml version=\"1.0\" encoding=\"UTF-8\"?>");
    UnicodeString SessionName =
      (FSessionData != nullptr) ? XmlAttributeEscape(FSessionData->SessionName) : UnicodeString(L"nosession");
    Add(FORMAT(L"<session xmlns=\"http://winscp.net/schema/session/1.0\" name=\"%s\" start=\"%s\">",
        (SessionName, StandardTimestamp())));
#endif // defined(__BORLANDC__)
  }
  else if (!ALogging && FLogging)
  {
    if (FInGroup)
    {
      EndGroup();
    }
    // do not try to close the file, if it has not been opened, to avoid recursion
    if (FLogger != nullptr)
    {
#if defined(__BORLANDC__)
      Add(L"</session>");
#endif // defined(__BORLANDC__)
    }
    CloseLogFile();
    FLogging = false;
  }

}

void TActionLog::CloseLogFile()
{
  if (FLogger != nullptr)
  {
#if defined(__BORLANDC__)
    fclose((FILE *)FFile);
    FFile = nullptr;
#endif
    FLogger.reset();
  }
  FCurrentLogFileName.Clear();
  FCurrentFileName.Clear();
}

void TActionLog::OpenLogFile()
{
  if (!FConfiguration)
    return;
  try
  {
    DebugAssert(FFile == nullptr);
    DebugAssert(FConfiguration != nullptr);
    DebugAssert(FLogger == nullptr);
    FCurrentLogFileName = FConfiguration->GetActionsLogFileName();
    FILE * file = ::OpenLogFile(FCurrentLogFileName, FStarted, FSessionData, false, FCurrentFileName);
    FLogger = std::make_unique<tinylog::TinyLog>();
    FLogger->file(file);
  }
  catch(Exception & E)
  {
    // We failed logging to file, turn it off and notify user.
    FCurrentLogFileName.Clear();
    FCurrentFileName.Clear();
    FConfiguration->SetLogActions(false);
    if (FConfiguration->GetLogActionsRequired())
    {
      throw EFatal(&E, LoadStr(LOG_FATAL_ERROR));
    }
    else
    {
      try
      {
        throw ExtException(&E, LoadStr(LOG_GEN_ERROR));
      }
      catch(Exception & E2)
      {
        if (FUI != nullptr)
        {
          // not to deadlock with TSessionLog::ReflectSettings invoked by FConfiguration->LogFileName setter above
          const TUnguard Unguard(FCriticalSection);
          FUI->HandleExtendedException(&E2);
        }
      }
    }
  }
}

void TActionLog::AddPendingAction(TSessionActionRecord * Action)
{
  FPendingActions->Add(Action);
}

void TActionLog::RecordPendingActions()
{
  while ((FPendingActions->GetCount() > 0) &&
    FPendingActions->GetAs<TSessionActionRecord>(0)->Record())
  {
    FPendingActions->Delete(0);
  }
}

void TActionLog::BeginGroup(const UnicodeString & Name)
{
  DebugAssert(!FInGroup);
  FInGroup = true;
  DebugAssert(FIndent == L"  ");
  AddIndented(FORMAT("<group name=\"%s\" start=\"%s\">",
    XmlAttributeEscape(Name), StandardTimestamp()));
  FIndent = L"    ";
}

void TActionLog::EndGroup()
{
  DebugAssert(FInGroup);
  FInGroup = false;
  DebugAssert(FIndent == L"    ");
  FIndent = L"  ";
  // this is called from ReflectSettings that in turn is called when logging fails,
  // so do not try to close the group, if it has not been opened, to avoid recursion
  if (FLogger != nullptr)
  {
    AddIndented("</group>");
  }
}

void TActionLog::SetEnabled(bool Value)
{
  if (GetEnabled() != Value)
  {
    FEnabled = Value;
    ReflectSettings();
  }
}


TApplicationLog::TApplicationLog()
{
  FFile = nullptr;
  FLogging = false;
  FPeekReservedMemory = 0;
#if defined(__BORLANDC__)
  FCriticalSection.reset(new TCriticalSection());
#endif // defined(__BORLANDC__)
}

TApplicationLog::~TApplicationLog()
{
  try { if (FFile != nullptr)
  {
    Log(L"Closing log");
    fclose(static_cast<FILE *>(FFile));
    FFile = nullptr;
  } } catch(const std::exception &) { DEBUG_PRINTF("Error"); }
  FLogging = false;
}

void TApplicationLog::Enable(const UnicodeString & Path)
{
  UnicodeString Dummy;
  FPath = Path;
  FFile = OpenLogFile(FPath, Now(), nullptr, false, Dummy);
  FLogging = true;
}

void TApplicationLog::AddStartupInfo()
{
  if (Logging)
  {
    // TODO: implement
#if defined(__BORLANDC__)
    TSessionLog::DoAddStartupInfo(nb::bind(&TApplicationLog::Log, this), Configuration, true);
#endif // defined(__BORLANDC__)
  }
}

void TApplicationLog::Log(const UnicodeString & S)
{
  if (FFile != nullptr)
  {
    TDateTime N = Now();
    const UnicodeString Timestamp = FormatDateTime(L"yyyy-mm-dd hh:nn:ss.zzz", N);
    const UnicodeString Line = FORMAT(L"[%s] [%x] %s\r\n", Timestamp, nb::ToInt32(GetCurrentThreadId()), S);
    const UTF8String UtfLine = UTF8String(Line);
    const int32_t Writing = UtfLine.Length();

    bool CheckMemory;

    {
      const TGuard Guard(FCriticalSection);
      fwrite(UtfLine.c_str(), 1, Writing, static_cast<FILE *>(FFile));
      int64_t SecondsSinceLastMemoryCheck = SecondsBetween(N, FLastMemoryCheck);
      CheckMemory = (SecondsSinceLastMemoryCheck >= 10);
      if (CheckMemory)
      {
        FLastMemoryCheck = N;
      }
    }

    if (CheckMemory)
    {
      BYTE * Address = NULL;
      MEMORY_BASIC_INFORMATION MemoryInfo;
      size_t ReservedMemory = 0;
      size_t CommittedMemory = 0;
      while (VirtualQuery(Address, &MemoryInfo, sizeof(MemoryInfo)) == sizeof(MemoryInfo))
      {
        if ((MemoryInfo.State == MEM_RESERVE) || (MemoryInfo.State == MEM_COMMIT))
        {
          ReservedMemory += MemoryInfo.RegionSize;
        }
        if ((MemoryInfo.State == MEM_COMMIT) && (MemoryInfo.Type == MEM_PRIVATE))
        {
          CommittedMemory += MemoryInfo.RegionSize;
        }

        Address += MemoryInfo.RegionSize;
      }

      bool NewMemoryPeek;
      {
        TGuard Guard(FCriticalSection);
        const size_t Threshold = 10 * 1024 * 1024;
        NewMemoryPeek =
          ((ReservedMemory > FPeekReservedMemory) &&
           ((ReservedMemory - FPeekReservedMemory) > Threshold)) |
          ((CommittedMemory > FPeekCommittedMemory) &&
           ((CommittedMemory - FPeekCommittedMemory) > Threshold));
        if (NewMemoryPeek)
        {
          FPeekReservedMemory = ReservedMemory;
          FPeekCommittedMemory = CommittedMemory;
        }
      }

      if (NewMemoryPeek)
      {
        Log(FORMAT(L"Memory increased: Reserved address space: %s, Committed private: %s",
              (FormatNumber(__int64(ReservedMemory)), FormatNumber(__int64(CommittedMemory)))));
      }
    }
  }
}
