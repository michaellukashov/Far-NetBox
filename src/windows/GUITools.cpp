
#include <vcl.h>
#pragma hdrstop

#include <System.ShlObj.hpp>
#include <mshtmhst.h>
#include <Common.h>

#include "GUITools.h"
#include "WinConfiguration.h"
#include <TextsCore.h>
#include <TextsWin.h>
#include <CoreMain.h>
#include <SessionData.h>
#include <WinInterface.h>
#include <Interface.h>

#if 0
#include <TbxUtils.hpp>
#include <Math.hpp>
#include <WebBrowserEx.hpp>
#include <Tools.h>
#include "PngImageList.hpp"
#include <StrUtils.hpp>
#include <limits>
#include <Glyphs.h>
#include <PasTools.hpp>
#include <VCLCommon.h>
#include <WinApi.h>
#include <Vcl.ScreenTips.hpp>
#include <HistoryComboBox.hpp>
#include <vssym32.h>
#include <DateUtils.hpp>
#include <IOUtils.hpp>
#include <Queue.h>

#include "Animations96.h"
#include "Animations120.h"
#include "Animations144.h"
#include "Animations192.h"
#endif // #if 0

__removed #pragma package(smart_init)

extern UnicodeString PageantTool = "pageant.exe";
extern UnicodeString PuttygenTool = "puttygen.exe";

bool FindFile(UnicodeString &APath)
{
  bool Result = ::SysUtulsFileExists(ApiPath(APath));
  if (!Result)
  {
    UnicodeString ProgramFiles32 = ::IncludeTrailingBackslash(base::GetEnvVariable(L"ProgramFiles"));
    UnicodeString ProgramFiles64 = ::IncludeTrailingBackslash(base::GetEnvVariable(L"ProgramW6432"));
    if (!ProgramFiles32.IsEmpty() &&
      SameText(APath.SubString(1, ProgramFiles32.Length()), ProgramFiles32) &&
      !ProgramFiles64.IsEmpty())
    {
      UnicodeString Path64 =
        ProgramFiles64 + APath.SubString(ProgramFiles32.Length() + 1, APath.Length() - ProgramFiles32.Length());
      if (::SysUtulsFileExists(ApiPath(Path64)))
      {
        APath = Path64;
        Result = true;
      }
    }
  }

  if (!Result && SameText(ExtractFileName(Path), Path))
  {
    UnicodeString Paths = base::GetEnvVariable("PATH");
    if (Paths.Length() > 0)
    {
      UnicodeString NewPath = ::SysUtulsFileSearch(base::ExtractFileName(APath, false), Paths);
      Result = !NewPath.IsEmpty();
      if (Result)
      {
        APath = NewPath;
      }
      else
      {
        // Basically the same what FileSearch does, except for FileExistsFix.
        // Once this proves working, we can ged rid of the FileSearch call.
        while (!Result && !Paths.IsEmpty())
        {
          UnicodeString P = CutToChar(Paths, L';', false);
          // Not using TPath::Combine as it throws on an invalid path and PATH is not under our control
          NewPath = IncludeTrailingBackslash(P) + Path;
          Result = FileExistsFix(NewPath);
          if (Result)
          {
            Path = NewPath;
          }
        }
      }
    }
  }
  return Result;
}

bool DoesSessionExistInPutty(TSessionData * SessionData)
{
  std::unique_ptr<TRegistryStorage> Storage(new TRegistryStorage(Configuration->PuttySessionsKey));
  Storage->ConfigureForPutty();
  return Storage->OpenRootKey(true) && Storage->KeyExists(SessionData->StorageKey);
}

bool ExportSessionToPutty(TSessionData * SessionData, bool ReuseExisting, const UnicodeString & SessionName)
{
  bool Result = true;
  std::unique_ptr<TRegistryStorage> Storage(new TRegistryStorage(Configuration->PuttySessionsKey));
  Storage->AccessMode = smReadWrite;
  Storage->ConfigureForPutty();
  if (Storage->OpenRootKey(true))
  {
    Result = ReuseExisting && Storage->KeyExists(SessionData->StorageKey);
    if (!Result)
    {
      std::unique_ptr<TRegistryStorage> SourceStorage(new TRegistryStorage(Configuration->PuttySessionsKey));
      SourceStorage->ConfigureForPutty();
      if (SourceStorage->OpenSubKey(StoredSessions->DefaultSettings->Name, false) &&
          Storage->OpenSubKey(SessionName, true))
      {
        Storage->Copy(SourceStorage.get());
        Storage->CloseSubKey();
      }

      std::unique_ptr<TSessionData> ExportData(new TSessionData(L""));
      ExportData->Assign(SessionData);
      ExportData->Modified = true;
      ExportData->Name = SessionName;
      ExportData->WinTitle = SessionData->SessionName;
      ExportData->Password = L"";

      if (SessionData->FSProtocol == fsFTP)
      {
        if (GUIConfiguration->TelnetForFtpInPutty)
        {
          ExportData->PuttyProtocol = PuttyTelnetProtocol;
          ExportData->PortNumber = TelnetPortNumber;
        }
        else
        {
          ExportData->PuttyProtocol = PuttySshProtocol;
          ExportData->PortNumber = SshPortNumber;
        }
      }

      ExportData->Save(Storage.get(), true);
    }
  }
  return Result;
}

void OpenSessionInPutty(UnicodeString PuttyPath,
  TSessionData *SessionData)
{
  UnicodeString Program, Params, Dir;
  SplitCommand(PuttyPath, Program, Params, Dir);
  Program = ::ExpandEnvironmentVariables(Program);
  if (FindFile(Program))
  {

    Params = ::ExpandEnvironmentVariables(Params);
    UnicodeString Password;
    if (GetGUIConfiguration()->PuttyPassword)
    {
      // Passphrase has precendence, as it's more likely entered by user during authentication, hence more likely really needed.
      if (!SessionData->Passphrase().IsEmpty())
      {
        Password = SessionData->Passphrase;
      }
      else if (!SessionData->Password().IsEmpty())
      {
        Password = SessionData->Password;
      }
    }
    TCustomCommandData Data(SessionData, SessionData->UserName, Password);
    TLocalCustomCommand LocalCustomCommand(Data, SessionData->RemoteDirectory, SessionData->LocalDirectory);
    TWinInteractiveCustomCommand InteractiveCustomCommand(
      &LocalCustomCommand, L"PuTTY", UnicodeString());

    UnicodeString Params2 =
      RemoteCustomCommand.Complete(InteractiveCustomCommand.Complete(Params, false), true);
    UnicodeString PuttyParams;

    if (!RemoteCustomCommand.IsSiteCommand(Params))
    {
      {
        bool SessionList = false;
        std::unique_ptr<THierarchicalStorage> SourceHostKeyStorage(GetConfiguration()->CreateStorage(SessionList));
        SourceHostKeyStorage->Init();
        std::unique_ptr<THierarchicalStorage> TargetHostKeyStorage(std::make_unique<TRegistryStorage>(GetConfiguration()->PuttyRegistryStorageKey()));
        TargetHostKeyStorage->Init();
        TargetHostKeyStorage->SetExplicit(true);
        TargetHostKeyStorage->SetAccessMode(smReadWrite);
        std::unique_ptr<TStoredSessionList> HostKeySessionList(std::make_unique<TStoredSessionList>());
        HostKeySessionList->SetOwnsObjects(false);
        HostKeySessionList->Add(SessionData);
        TStoredSessionList::ImportHostKeys(SourceHostKeyStorage.get(), TargetHostKeyStorage.get(), HostKeySessionList.get(), false);
      }

      if (IsUWP())
      {
        bool Telnet = (SessionData->FSProtocol() == fsFTP) && GetGUIConfiguration()->TelnetForFtpInPutty;
        if (Telnet)
        {
          AddToList(PuttyParams, L"-telnet", L" ");
          // PuTTY  does not allow -pw for telnet
          Password = L"";
        }
        AddToList(PuttyParams, EscapePuttyCommandParam(SessionData->HostName()), L" ");
        if (!SessionData->UserName().IsEmpty())
        {
          AddToList(PuttyParams, FORMAT("-l %s", EscapePuttyCommandParam(SessionData->UserName())), L" ");
        }
        if ((SessionData->FSProtocol() != fsFTP) && (SessionData->PortNumber() != SshPortNumber))
        {
          AddToList(PuttyParams, FORMAT("-P %d", SessionData->PortNumber()), L" ");
        }

        if (!Telnet)
        {
          if (!SessionData->PublicKeyFile().IsEmpty())
          {
            AddToList(PuttyParams, FORMAT("-i \"%s\"", SessionData->PublicKeyFile()), L" ");
          }
          AddToList(PuttyParams, (SessionData->TryAgent() ? L"-agent" : L"-noagent"), L" ");
          if (SessionData->TryAgent())
          {
            AddToList(PuttyParams, (SessionData->AgentFwd() ? L"-A" : L"-a"), L" ");
          }
          if (SessionData->Compression())
          {
            AddToList(PuttyParams, L"-C", L" ");
          }
          AddToList(PuttyParams, L"-2", L" ");
          if (!SessionData->LogicalHostName().IsEmpty())
          {
            AddToList(PuttyParams, FORMAT("-loghost \"%s\"", SessionData->LogicalHostName()), L" ");
          }
        }

        if (SessionData->AddressFamily == afIPv4)
        {
          AddToList(PuttyParams, L"-4", L" ");
        }
        else if (SessionData->AddressFamily == afIPv6)
        {
          AddToList(PuttyParams, L"-6", L" ");
        }
      }
      else
      {
        UnicodeString SessionName;
        std::unique_ptr<TRegistryStorage> Storage = nullptr;
        std::unique_ptr<TSessionData> ExportData = nullptr;
        std::unique_ptr<TRegistryStorage> SourceStorage = nullptr;
        try__finally
          Storage = std::make_unique<TRegistryStorage>(GetConfiguration()->GetPuttySessionsKey());
          Storage->Init();
          Storage->SetAccessMode(smReadWrite);
          Storage->SetMungeStringValues(false);
          Storage->SetForceAnsi(true);
        {
          SessionName = SessionData->SessionName;
        }
        else
        {
              SourceStorage = std::make_unique<TRegistryStorage>(GetConfiguration()->PuttySessionsKey);
              SourceStorage->Init();
              GUIConfiguration->TelnetForFtpInPutty)
                  Storage->OpenSubKey(GetGUIConfiguration()->PuttySession, true))
                Storage->Copy(SourceStorage.get());
              ExportData = std::make_unique<TSessionData>(L"");
              ExportData->SetModified(true);
              ExportData->SetName(GetGUIConfiguration()->PuttySession);
              ExportData->SetWinTitle(SessionData->SessionName);
              ExportData->SetPassword(L"");
              if (SessionData->FSProtocol() == fsFTP)
                if (GetGUIConfiguration()->TelnetForFtpInPutty)
          {
                  ExportData->SetPuttyProtocol(PuttyTelnetProtocol);
            // PuTTY  does not allow -pw for telnet
            Password = L"";
          }
                  ExportData->SetPuttyProtocol(PuttySshProtocol);
              ExportData->Save(Storage.get(), true);
              SessionName = GetGUIConfiguration()->PuttySession;
            }
          }
        },
        __finally__removed
        ({
        }) end_try__finally

        UnicodeString LoadSwitch = L"-load";
        intptr_t P = Params2.LowerCase().Pos(LoadSwitch + L" ");
        if ((P == 0) || ((P > 1) && (Params2[P - 1] != L' ')))
        {
          AddToList(PuttyParams, FORMAT("%s %s", LoadSwitch, EscapePuttyCommandParam(SessionName)), L" ");
        }
      }
    }

    if (!Password.IsEmpty() && !RemoteCustomCommand.IsPasswordCommand(Params))
    {
      Password = NormalizeString(Password); // if password is empty, we should quote it always
      AddToList(PuttyParams, FORMAT("-pw %s", EscapePuttyCommandParam(Password)), L" ");
    }

    AddToList(PuttyParams, Params2, L" ");

    // PuTTY is started in its binary directory to allow relative paths in private key,
    // when opening PuTTY's own stored session.
    ExecuteShellChecked(Program, PuttyParams, true);
  }
  else
  {
    throw Exception(FMTLOAD(FILE_NOT_FOUND, Program));
  }
}

bool FindTool(UnicodeString Name, UnicodeString &APath)
{
  UnicodeString AppPath = ::IncludeTrailingBackslash(::ExtractFilePath(GetConfiguration()->ModuleFileName()));
  APath = AppPath + Name;
  bool Result = true;
  if (!::SysUtulsFileExists(ApiPath(APath)))
  {
    APath = AppPath + L"PuTTY\\" + Name;
    if (!::SysUtulsFileExists(ApiPath(APath)))
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

void ExecuteTool(UnicodeString AName)
{
  UnicodeString Path;
  if (!FindTool(AName, Path))
  {
    throw Exception(FMTLOAD(EXECUTE_APP_ERROR, AName));
  }

  ExecuteShellChecked(Path, L"");
}

#if 0
TObjectList * StartCreationDirectoryMonitorsOnEachDrive(uintptr_t Filter, TFileChangedEvent OnChanged)
{
  std::unique_ptr<TStrings> Drives(std::make_unique<TStringList>());

  std::unique_ptr<TStrings> DDDrives(std::make_unique<TStringList>());
  DDDrives->CommaText = WinConfiguration->DDDrives;
  UnicodeString ExcludedDrives;
  for (int Index = 0; Index < DDDrives->Count; Index++)
  {
    UnicodeString S = Trim(DDDrives->Strings[Index]);
    if (!S.IsEmpty() && (S[1] == L'-'))
    {
      S = Trim(S.SubString(2, S.Length() - 1));
      if (!S.IsEmpty())
      {
        ExcludedDrives += S[1];
      }
    }
    else
    {
      Drives->Add(S);
    }
  }

  for (char Drive = FirstDrive; Drive <= LastDrive; Drive++)
  {
    if (ExcludedDrives.Pos(Drive) == 0)
    {
      // Not calling ReadDriveStatus(... dsSize), relying on drive ready status cached by the background thread
      TDriveInfoRec * DriveInfoRec = DriveInfo->Get(Drive);
      if (DriveInfoRec->Valid && DriveInfoRec->DriveReady &&
          (DriveInfoRec->DriveType != DRIVE_CDROM) &&
          ((DriveInfoRec->DriveType != DRIVE_REMOVABLE) || (Drive >= DriveInfo->FirstFixedDrive)))
      {
        Drives->Add(Drive);
      }
    }
  }

  std::unique_ptr<TObjectList> Result(std::make_unique<TObjectList>());
  for (int Index = 0; Index < Drives->Count; Index++)
  {
    UnicodeString Drive = Drives->Strings[Index];
    std::unique_ptr<TDirectoryMonitor> Monitor(std::make_unique<TDirectoryMonitor>(Application));
    try
    {
      Monitor->Path = DriveInfo->GetDriveRoot(Drive);
      Monitor->WatchSubtree = true;
      Monitor->WatchFilters = Filter;
      Monitor->OnCreated = OnChanged;
      Monitor->OnModified = OnChanged;
      Monitor->Active = true;
      Result->Add(Monitor.release());
    }
    catch (Exception & E)
    {
      // Ignore errors watching not-ready drives
    }
  }
  return Result.release();
}
#endif // #if 0

bool DontCopyCommandToClipboard = false;

bool CopyCommandToClipboard(UnicodeString ACommand)
{
  bool Result = !DontCopyCommandToClipboard; // && UseAlternativeFunction && IsKeyPressed(VK_CONTROL);
  if (Result)
  {
    TInstantOperationVisualizer Visualizer; nb::used(Visualizer);
    CopyToClipboard(ACommand);
  }
  return Result;
}

static bool DoExecuteShell(UnicodeString APath, UnicodeString Params,
  bool ChangeWorkingDirectory, HANDLE *Handle)
{
  bool Result = CopyCommandToClipboard(FormatCommand(APath, Params));

  if (Result)
  {
    if (Handle != nullptr)
    {
      *Handle = nullptr;
    }
  }
  else
  {
    UnicodeString Directory = ::ExtractFilePath(APath);

    TShellExecuteInfoW ExecuteInfo;
    nb::ClearStruct(ExecuteInfo);
    ExecuteInfo.cbSize = sizeof(ExecuteInfo);
    ExecuteInfo.fMask =
      SEE_MASK_FLAG_NO_UI |
      FLAGMASK((Handle != nullptr), SEE_MASK_NOCLOSEPROCESS);
    ExecuteInfo.hwnd = reinterpret_cast<HWND>(::GetModuleHandle(nullptr));
    ExecuteInfo.lpFile = ToWChar(APath);
    ExecuteInfo.lpParameters = ToWChar(Params);
    ExecuteInfo.lpDirectory = (ChangeWorkingDirectory ? Directory.c_str() : nullptr);
    ExecuteInfo.nShow = SW_SHOW;

    Result = (::ShellExecuteEx(&ExecuteInfo) != 0);
    if (Result)
    {
      if (Handle != nullptr)
      {
        *Handle = ExecuteInfo.hProcess;
      }
    }
  }
  return Result;
}

void ExecuteShellChecked(UnicodeString APath, UnicodeString Params, bool ChangeWorkingDirectory)
{
  if (!DoExecuteShell(APath, Params, ChangeWorkingDirectory, nullptr))
  {
    throw EOSExtException(FMTLOAD(EXECUTE_APP_ERROR, APath));
  }
}

void ExecuteShellChecked(UnicodeString Command)
{
  UnicodeString Program, Params, Dir;
  SplitCommand(Command, Program, Params, Dir);
  ExecuteShellChecked(Program, Params);
}

bool ExecuteShell(UnicodeString Path, UnicodeString Params,
  HANDLE &Handle)
{
  return DoExecuteShell(Path, Params, false, &Handle);
}

void ExecuteShellCheckedAndWait(UnicodeString Command,
  TProcessMessagesEvent ProcessMessages)
{
  UnicodeString Program, Params, Dir;
  SplitCommand(Command, Program, Params, Dir);
  HANDLE ProcessHandle;
  bool Result = DoExecuteShell(Program, Params, false, &ProcessHandle);
  if (!Result)
  {
    throw EOSExtException(FMTLOAD(EXECUTE_APP_ERROR, Program));
  }
  else
  {
    if (ProcessHandle != nullptr) // only if command was copied to clipboard only
    {
      if (ProcessMessages != nullptr)
      {
        unsigned long WaitResult;
        do
        {
          // Same as in ExecuteProcessAndReadOutput
          WaitResult = WaitForSingleObject(ProcessHandle, 50);
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
        WaitForSingleObject(ProcessHandle, INFINITE);
      }
    }
  }
}

bool SpecialFolderLocation(intptr_t PathID, UnicodeString &APath)
{
#if defined(_MSC_VER)
  LPITEMIDLIST Pidl;
  wchar_t Buf[MAX_PATH];
  if (::SHGetSpecialFolderLocation(nullptr, nb::ToInt(PathID), &Pidl) == NO_ERROR &&
    ::SHGetPathFromIDList(Pidl, Buf))
  {
    APath = UnicodeString(Buf);
    return true;
  }
#endif // if defined(_MSC_VER)
  return false;
}

UnicodeString UniqTempDir(UnicodeString BaseDir, UnicodeString Identity,
  bool Mask)
{
  DebugAssert(!BaseDir.IsEmpty());
  UnicodeString TempDir;
  do
  {
    TempDir = ::IncludeTrailingBackslash(BaseDir) + Identity;
    if (Mask)
    {
      TempDir += L"?????";
    }
    else
    {
      TempDir += ::IncludeTrailingBackslash(FormatDateTime(L"nnzzz", Now()));
    }
  }
  while (!Mask && ::SysUtulsDirectoryExists(ApiPath(TempDir)));

  return TempDir;
}

bool DeleteDirectory(UnicodeString ADirName)
{
  TSearchRecOwned sr;
  bool retval = true;
  if (FindFirstUnchecked(ADirName + L"\\*", faAnyFile, sr) == 0) // VCL Function
  {
    if (sr.IsDirectory())
    {
      if (sr.IsRealFile())
        retval = ::DeleteDirectory(ADirName + L"\\" + sr.Name);
    }
    else
    {
      retval = ::SysUtulsRemoveFile(ApiPath(ADirName + L"\\" + sr.Name));
    }

    if (retval)
    {
      while (FindNextChecked(sr) == 0)
      {
        if (sr.IsDirectory())
        {
          if (sr.IsRealFile())
            retval = DeleteDirectory(ADirName + L"\\" + sr.Name);
        }
        else
        {
          retval = ::SysUtulsRemoveFile(ApiPath(ADirName + L"\\" + sr.Name));
        }

        if (!retval) break;
      }
    }
  }
  sr.Close();
  if (retval) retval = ::SysUtulsRemoveDir(ApiPath(ADirName)); // VCL function
  return retval;
}
#if 0

class TSessionColors : public TComponent
{
public:
  TSessionColors(TComponent * Owner) : TComponent(Owner)
  {
    Name = QualifiedClassName();
  }

  static TSessionColors * Retrieve(TComponent * Component)
  {
    TSessionColors * SessionColors = dynamic_cast<TSessionColors *>(Component->FindComponent(QualifiedClassName()));
    if (SessionColors == NULL)
    {
      SessionColors = new TSessionColors(Component);
    }
    return SessionColors;
  }

  typedef std::map<TColor, int> TColorMap;
  TColorMap ColorMap;
};

int GetSessionColorImage(
{

  TSessionColors * SessionColors = TSessionColors::Retrieve(ImageList);

  int Result;
  TSessionColors::TColorMap::const_iterator I = SessionColors->ColorMap.find(Color);
  if (I != SessionColors->ColorMap.end())
  {
    Result = I->second;
  }
  else
  {
    // This overly complex drawing is here to support color button on SiteAdvanced
    // dialog. There we use plain TImageList, instead of TPngImageList,
    // TButton does not work with transparent images
    // (not even TBitmap with Transparent = true)
    std::unique_ptr<TBitmap> MaskBitmap(new TBitmap());
    ImageList->GetBitmap(MaskIndex, MaskBitmap.get());

    std::unique_ptr<TPngImage> MaskImage(new TPngImage());
    MaskImage->Assign(MaskBitmap.get());

    std::unique_ptr<TPngImage> ColorImage(new TPngImage(COLOR_RGB, 16, ImageList->Width, ImageList->Height));

    TColor MaskTransparentColor = MaskImage->Pixels[0][0];
    TColor TransparentColor = MaskTransparentColor;
    // Expecting that the color to be replaced is in the centre of the image (HACK)
    TColor MaskColor = MaskImage->Pixels[ImageList->Width / 2][ImageList->Height / 2];

    for (int Y = 0; Y < ImageList->Height; Y++)
    {
      for (int X = 0; X < ImageList->Width; X++)
      {
        TColor SourceColor = MaskImage->Pixels[X][Y];
        TColor DestColor;
        // this branch is pointless as long as MaskTransparentColor and
        // TransparentColor are the same
        if (SourceColor == MaskTransparentColor)
        {
          DestColor = TransparentColor;
        }
        else if (SourceColor == MaskColor)
        {
          DestColor = Color;
        }
        else
        {
          DestColor = SourceColor;
        }
        ColorImage->Pixels[X][Y] = DestColor;
      }
    }

    std::unique_ptr<TBitmap> Bitmap(new TBitmap());
    Bitmap->SetSize(ImageList->Width, ImageList->Height);
    ColorImage->AssignTo(Bitmap.get());

    Result = ImageList->AddMasked(Bitmap.get(), TransparentColor);

    SessionColors->ColorMap.insert(std::make_pair(Color, Result));
  }
  return Result;
}

void RegenerateSessionColorsImageList(TCustomImageList * ImageList, int MaskIndex)
{
  TSessionColors * SessionColors = TSessionColors::Retrieve(ImageList);

  std::vector<TColor> Colors;
  size_t FixedImages = static_cast<size_t>(ImageList->Count);
  Colors.resize(FixedImages + SessionColors->ColorMap.size());
  TSessionColors::TColorMap::const_iterator I = SessionColors->ColorMap.begin();
  while (I != SessionColors->ColorMap.end())
  {
    DebugAssert(Colors[I->second] == TColor());
    Colors[I->second] = I->first;
    I++;
  }

  TSessionColors::TColorMap ColorMap = SessionColors->ColorMap;
  SessionColors->ColorMap.clear();

  for (size_t Index = 0; Index < Colors.size(); Index++)
  {
    bool IsFixedImageIndex = (Index < FixedImages);
    DebugAssert((Colors[Index] == TColor()) == IsFixedImageIndex);
    if (!IsFixedImageIndex)
    {
      GetSessionColorImage(ImageList, Colors[Index], MaskIndex);
    }
  }

  DebugAssert(SessionColors->ColorMap == ColorMap);
}

void RegenerateSessionColorsImageList(TCustomImageList * ImageList, int MaskIndex)
{
  TSessionColors * SessionColors = TSessionColors::Retrieve(ImageList);

  std::vector<TColor> Colors;
  size_t FixedImages = static_cast<size_t>(ImageList->Count);
  Colors.resize(FixedImages + SessionColors->ColorMap.size());
  TSessionColors::TColorMap::const_iterator I = SessionColors->ColorMap.begin();
  while (I != SessionColors->ColorMap.end())
  {
    DebugAssert(Colors[I->second] == TColor());
    Colors[I->second] = I->first;
    I++;
  }

  TSessionColors::TColorMap ColorMap = SessionColors->ColorMap;
  SessionColors->ColorMap.clear();

  for (size_t Index = 0; Index < Colors.size(); Index++)
  {
    bool IsFixedImageIndex = (Index < FixedImages);
    DebugAssert((Colors[Index] == TColor()) == IsFixedImageIndex);
    if (!IsFixedImageIndex)
    {
      GetSessionColorImage(ImageList, Colors[Index], MaskIndex);
    }
  }

  DebugAssert(SessionColors->ColorMap == ColorMap);
}

void SetSubmenu(TTBXCustomItem * Item)
{
  class TTBXPublicItem : public TTBXCustomItem
  {
  public:
    __property ItemStyle;
  };
  TTBXPublicItem * PublicItem = reinterpret_cast<TTBXPublicItem *>(Item);
  DebugAssert(PublicItem != NULL);
  // See TTBItemViewer.IsPtInButtonPart (called from TTBItemViewer.MouseDown)
  PublicItem->ItemStyle = PublicItem->ItemStyle << tbisSubmenu;
}

bool IsEligibleForApplyingTabs(
  UnicodeString Line, int & TabPos, UnicodeString & Start, UnicodeString & Remaining)
{
  bool Result = false;
  TabPos = Line.Pos(L"\t");
  if (TabPos > 0)
  {
    Remaining = Line.SubString(TabPos + 1, Line.Length() - TabPos);
    // WORKAROUND
    // Some translations still use obsolete hack of consecutive tabs to aling the contents.
    // Delete these, so that the following check does not fail on this
    while (Remaining.SubString(1, 1) == L"\t")
    {
      Remaining.Delete(1, 1);
    }

    // We do not have, not support, mutiple tabs on a single line
    if (DebugAlwaysTrue(Remaining.Pos(L"\t") == 0))
    {
      Start = Line.SubString(1, TabPos - 1);
      // WORKAROUND
      // Previously we padded the string before tab with spaces,
      // to aling the contents across multiple lines
      Start = Start.TrimRight();
      // at least two normal spaces for separation
      Start += L"  ";
      Result = true;
    }
  }
  return Result;
}

static int CalculateWidthByLength(UnicodeString Text, void * /*Arg*/)
{
  return Text.Length();
}

void ApplyTabs(
  UnicodeString & Text, wchar_t Padding,
  TCalculateWidth CalculateWidth, void * CalculateWidthArg)
{
  if (CalculateWidth == NULL)
  {
    DebugAssert(CalculateWidthArg == NULL);
    CalculateWidth = CalculateWidthByLength;
  }

  std::unique_ptr<TStringList> Lines(TextToStringList(Text));

  int MaxWidth = -1;
  for (int Index = 0; Index < Lines->Count; Index++)
  {
    UnicodeString Line = Lines->Strings[Index];
    int TabPos;
    UnicodeString Start;
    UnicodeString Remaining;
    if (IsEligibleForApplyingTabs(Line, TabPos, Start, Remaining))
    {
      int Width = CalculateWidth(Start, CalculateWidthArg);
      MaxWidth = Max(MaxWidth, Width);
    }
  }

  // Optimization and also to prevent potential regression for texts without tabs
  if (MaxWidth >= 0)
  {
    for (int Index = 0; Index < Lines->Count; Index++)
    {
      UnicodeString Line = Lines->Strings[Index];
      int TabPos;
      UnicodeString Start;
      UnicodeString Remaining;
      if (IsEligibleForApplyingTabs(Line, TabPos, Start, Remaining))
      {
        int Width;
        int Iterations = 0;
        while ((Width = CalculateWidth(Start, CalculateWidthArg)) < MaxWidth)
        {
          int Wider = CalculateWidth(Start + Padding, CalculateWidthArg);
          // If padded string is wider than max width by more pixels
          // than non-padded string is shorter than max width
          if ((Wider > MaxWidth) && ((Wider - MaxWidth) > (MaxWidth - Width)))
          {
            break;
          }
          Start += Padding;
          Iterations++;
          // In rare case a tab is zero-width with some strange font (like HYLE)
          if (Iterations > 100)
          {
            break;
          }
        }
        Lines->Strings[Index] = Start + Remaining;
      }
    }

    Text = Lines->Text;
    // remove trailing newline
    Text = Text.TrimRight();
  }
}

static void DoSelectScaledImageList(TImageList * ImageList)
{
  TImageList * MatchingList = NULL;
  int MachingPixelsPerInch = 0;
  int PixelsPerInch = GetComponentPixelsPerInch(ImageList);

  for (int Index = 0; Index < ImageList->Owner->ComponentCount; Index++)
  {
    TImageList * OtherList = dynamic_cast<TImageList *>(ImageList->Owner->Components[Index]);

    if ((OtherList != NULL) &&
        (OtherList != ImageList) &&
        StartsStr(ImageList->Name, OtherList->Name))
    {
      UnicodeString OtherListPixelsPerInchStr =
        OtherList->Name.SubString(
          ImageList->Name.Length() + 1, OtherList->Name.Length() - ImageList->Name.Length());
      int OtherListPixelsPerInch = StrToInt(OtherListPixelsPerInchStr);
      if ((OtherListPixelsPerInch <= PixelsPerInch) &&
          ((MatchingList == NULL) ||
           (MachingPixelsPerInch < OtherListPixelsPerInch)))
      {
        MatchingList = OtherList;
        MachingPixelsPerInch = OtherListPixelsPerInch;
      }
    }
  }

  if (MatchingList != NULL)
  {
    UnicodeString ImageListBackupName = ImageList->Name + IntToStr(USER_DEFAULT_SCREEN_DPI);

    if (ImageList->Owner->FindComponent(ImageListBackupName) == NULL)
    {
      TImageList * ImageListBackup;
      TPngImageList * PngImageList = dynamic_cast<TPngImageList *>(ImageList);
      if (PngImageList != NULL)
      {
        ImageListBackup = new TPngImageList(ImageList->Owner);
      }
      else
      {
        ImageListBackup = new TImageList(ImageList->Owner);
      }

      ImageListBackup->Name = ImageListBackupName;
      ImageList->Owner->InsertComponent(ImageListBackup);
      CopyImageList(ImageListBackup, ImageList);
    }

    CopyImageList(ImageList, MatchingList);
  }
}

static void ImageListRescale(TComponent * Sender, TObject * /*Token*/)
{
  TImageList * ImageList = DebugNotNull(dynamic_cast<TImageList *>(Sender));
  DoSelectScaledImageList(ImageList);
}

void SelectScaledImageList(TImageList * ImageList)
{
  DoSelectScaledImageList(ImageList);

  SetRescaleFunction(ImageList, ImageListRescale);
}

void CopyImageList(TImageList * TargetList, TImageList * SourceList)
{
  // Maybe this is not necessary, once the TPngImageList::Assign was fixed
  TPngImageList * PngTargetList = dynamic_cast<TPngImageList *>(TargetList);
  TPngImageList * PngSourceList = dynamic_cast<TPngImageList *>(SourceList);

  TargetList->Clear();
  TargetList->Height = SourceList->Height;
  TargetList->Width = SourceList->Width;

  if ((PngTargetList != NULL) && (PngSourceList != NULL))
  {
    // AddImages won't copy the names and we need them for
    // LoadDialogImage and TFrameAnimation
    PngTargetList->PngImages->Assign(PngSourceList->PngImages);
  }
  else
  {
    TargetList->AddImages(SourceList);
  }
}

static bool DoLoadDialogImage(TImage * Image, UnicodeString & ImageName)
{
  bool Result = false;
  if (GlyphsModule != NULL)
  {
    TPngImageList * DialogImages = GetDialogImages(Image);

    int Index;
    for (Index = 0; Index < DialogImages->PngImages->Count; Index++)
    {
      TPngImageCollectionItem * PngItem = DialogImages->PngImages->Items[Index];
      if (SameText(PngItem->Name, ImageName))
      {
        Image->Picture->Assign(PngItem->PngImage);
        break;
      }
    }

    DebugAssert(Index < DialogImages->PngImages->Count);
    Result = true;
  }
  // When showing an exception from wWinMain, the images are released already.
  // Non-existence of the glyphs module is just a good indication of that.
  // We expect errors only.
  else if (ImageName == L"Error")
  {
    Image->Picture->Icon->Handle = LoadIcon(0, IDI_HAND);
  }
  // For showing an information about trace files
  else if (DebugAlwaysTrue(ImageName == L"Information"))
  {
    Image->Picture->Icon->Handle = LoadIcon(0, IDI_APPLICATION);
  }
  return Result;
}

class TDialogImageName : public TObject
{
public:
  UnicodeString ImageName;
};

static void DialogImageRescale(TComponent * Sender, TObject * Token)
{
  TImage * Image = DebugNotNull(dynamic_cast<TImage *>(Sender));
  TDialogImageName * DialogImageName = DebugNotNull(dynamic_cast<TDialogImageName *>(Token));
  DoLoadDialogImage(Image, DialogImageName->ImageName);
}

void LoadDialogImage(TImage * Image, UnicodeString & ImageName)
{
  if (DoLoadDialogImage(Image, ImageName))
  {
    TDialogImageName * DialogImageName = new TDialogImageName();
    DialogImageName->ImageName = ImageName;
    SetRescaleFunction(Image, DialogImageRescale, DialogImageName, true);
  }
}

int DialogImageSize(TForm * Form)
{
  return ScaleByPixelsPerInch(32, Form);
}

void HideComponentsPanel(TForm * Form)
{
  TComponent * Component = DebugNotNull(Form->FindComponent(L"ComponentsPanel"));
  TPanel * Panel = DebugNotNull(dynamic_cast<TPanel *>(Component));
  DebugAssert(Panel->Align == alBottom);
  int Offset = Panel->Height;
  Panel->Visible = false;
  Panel->Height = 0;
  Form->Height -= Offset;

  for (int Index = 0; Index < Form->ControlCount; Index++)
  {
    TControl * Control = Form->Controls[Index];

    // Shift back bottom-anchored controls
    // (needed for toolbar panel on Progress window and buttons on Preferences dialog),
    if ((Control->Align == alNone) &&
        Control->Anchors.Contains(akBottom) &&
        !Control->Anchors.Contains(akTop))
    {
      Control->Top += Offset;
    }

    // Resize back all-anchored controls
    // (needed for main panel on Preferences dialog),
    if (Control->Anchors.Contains(akBottom) &&
        Control->Anchors.Contains(akTop))
    {
      Control->Height += Offset;
    }
  }
}

UnicodeString FormatIncrementalSearchStatus(UnicodeString AText, bool HaveNext)
{
  UnicodeString Result =
    L" " + FMTLOAD(INC_SEARCH, AText) +
    (HaveNext ? L" " + LoadStr(INC_NEXT_SEARCH) : UnicodeString());
  return Result;
}


class TCustomDocHandler : public TComponent, public ::IDocHostUIHandler
{
public:
  TCustomDocHandler(TComponent * Owner) : TComponent(Owner)
  {
  }

protected:
  #pragma warn -hid
  virtual HRESULT STDMETHODCALLTYPE QueryInterface(REFIID ClassId, void ** Intf)
  {
    HRESULT Result = S_OK;
    if (ClassId == IID_IUnknown)
    {
      *Intf = (IUnknown *)this;
    }
    else if (ClassId == ::IID_IDocHostUIHandler)
    {
      *Intf = (::IDocHostUIHandler *)this;
    }
    else
    {
      Result = E_NOINTERFACE;
    }
    return Result;
  }
  #pragma warn .hid

  virtual ULONG STDMETHODCALLTYPE AddRef()
  {
    return -1;
  }

  virtual ULONG STDMETHODCALLTYPE Release()
  {
    return -1;
  }

  virtual HRESULT STDMETHODCALLTYPE ShowContextMenu(
    DWORD dwID, POINT * ppt, IUnknown * pcmdtReserved, IDispatch * pdispReserved)
  {
    // No context menu
    // (implementing IDocHostUIHandler reenabled context menu disabled by TBrowserViewer::DoContextPopup)
    return S_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE GetHostInfo(::_DOCHOSTUIINFO * Info)
  {
    // Setting ControlBorder is ignored with IDocHostUIHandler.
    // DOCHOSTUIFLAG_DPI_AWARE does not seem to have any effect
    Info->dwFlags |= DOCHOSTUIFLAG_SCROLL_NO | DOCHOSTUIFLAG_NO3DBORDER | DOCHOSTUIFLAG_DPI_AWARE;
    return S_OK;
  }

  virtual HRESULT STDMETHODCALLTYPE ShowUI(
    DWORD dwID, IOleInPlaceActiveObject * pActiveObject, IOleCommandTarget * pCommandTarget, IOleInPlaceFrame * pFrame,
    IOleInPlaceUIWindow * pDoc)
  {
    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE HideUI()
  {
    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE UpdateUI()
  {
    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE EnableModeless(BOOL fEnable)
  {
    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE OnDocWindowActivate(BOOL fActivate)
  {
    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE OnFrameWindowActivate(BOOL fActivate)
  {
    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE ResizeBorder(LPCRECT prcBorder, IOleInPlaceUIWindow * pUIWindow, BOOL fRameWindow)
  {
    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE TranslateAccelerator(LPMSG lpMsg, const GUID * pguidCmdGroup, DWORD nCmdID)
  {
    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE GetOptionKeyPath(LPOLESTR * pchKey, DWORD dw)
  {
    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE GetDropTarget(IDropTarget * pDropTarget, IDropTarget ** ppDropTarget)
  {
    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE GetExternal(IDispatch ** ppDispatch)
  {
    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE TranslateUrl(DWORD dwTranslate, OLECHAR * pchURLIn, OLECHAR ** ppchURLOut)
  {
    return E_NOTIMPL;
  }

  virtual HRESULT STDMETHODCALLTYPE FilterDataObject(IDataObject * pDO, IDataObject ** ppDORet)
  {
    return E_NOTIMPL;
  }
};

class TBrowserViewer : public TWebBrowserEx
{
public:
  virtual TBrowserViewer(TComponent* AOwner);

  void AddLinkHandler(
    UnicodeString & Url, TNotifyEvent Handler);
  void NavigateToUrl(UnicodeString & Url);

  TControl * LoadingPanel;

protected:
  DYNAMIC void DoContextPopup(const TPoint & MousePos, bool & Handled);
  void DocumentComplete(
    TObject * Sender, const _di_IDispatch Disp, const OleVariant & URL);
  void BeforeNavigate2(
    TObject * Sender, const _di_IDispatch Disp, const OleVariant & URL,
    const OleVariant & Flags, const OleVariant & TargetFrameName,
    const OleVariant & PostData, const OleVariant & Headers, WordBool & Cancel);

  bool FComplete;
  std::map<UnicodeString, TNotifyEvent> FHandlers;
};

TBrowserViewer::TBrowserViewer(TComponent* AOwner) :
  TWebBrowserEx(AOwner)
{
  FComplete = false;

  OnDocumentComplete = DocumentComplete;
  OnBeforeNavigate2 = BeforeNavigate2;
  LoadingPanel = NULL;
}

void TBrowserViewer::AddLinkHandler(
  UnicodeString & Url, TNotifyEvent Handler)
{
  FHandlers.insert(std::make_pair(Url, Handler));
}

void TBrowserViewer::DoContextPopup(const TPoint & MousePos, bool & Handled)
{
  // Suppress built-in context menu.
  // Is ignored with IDocHostUIHandler. Needs to be overriden by ShowContextMenu.
  Handled = true;
  TWebBrowserEx::DoContextPopup(MousePos, Handled);
}

void TBrowserViewer::DocumentComplete(
    TObject * /*Sender*/, const _di_IDispatch /*Disp*/, const OleVariant & /*URL*/)
{
  SetBrowserDesignModeOff(this);

  FComplete = true;

  if (LoadingPanel != NULL)
  {
    LoadingPanel->Visible = false;
  }
}

void TBrowserViewer::BeforeNavigate2(
  TObject * /*Sender*/, const _di_IDispatch /*Disp*/, const OleVariant & AURL,
  const OleVariant & /*Flags*/, const OleVariant & /*TargetFrameName*/,
  const OleVariant & /*PostData*/, const OleVariant & /*Headers*/, WordBool & Cancel)
{
  // If OnDocumentComplete was not called yet, is has to be our initial message URL,
  // opened using TWebBrowserEx::Navigate(), allow it.
  // Otherwise it's user navigating, block that and open link
  // in an external browser, possibly adding campaign parameters on the way.
  if (FComplete)
  {
    Cancel = 1;

    UnicodeString URL = AURL;
    if (FHandlers.count(URL) > 0)
    {
      FHandlers[URL](this);
    }
    else
    {
      OpenBrowser(URL);
    }
  }
}

void TBrowserViewer::NavigateToUrl(UnicodeString & Url)
{
  FComplete = false;
  Navigate(Url.c_str());
}

TPanel * CreateLabelPanel(TPanel * Parent, UnicodeString & Label)
{
  TPanel * Result = CreateBlankPanel(Parent);
  Result->Parent = Parent;
  Result->Align = alClient;
  Result->Caption = Label;
  return Result;
}

TWebBrowserEx * CreateBrowserViewer(TPanel * Parent, UnicodeString & LoadingLabel)
{
  TBrowserViewer * Result = new TBrowserViewer(Parent);
  // TWebBrowserEx has its own unrelated Name and Parent properties.
  // The name is used in DownloadUpdate().
  static_cast<TWinControl *>(Result)->Name = L"BrowserViewer";
  static_cast<TWinControl *>(Result)->Parent = Parent;
  Result->Align = alClient;
  // Is ignored with IDocHostUIHandler. Needs to be overriden by DOCHOSTUIFLAG_NO3DBORDER in GetHostInfo.
  Result->ControlBorder = cbNone;

  Result->LoadingPanel = CreateLabelPanel(Parent, LoadingLabel);

  return Result;
}

void SetBrowserDesignModeOff(TWebBrowserEx * WebBrowser)
{
  if (DebugAlwaysTrue(WebBrowser->Document2 != NULL))
  {
    WebBrowser->Document2->designMode = L"Off";
  }
}

void AddBrowserLinkHandler(TWebBrowserEx * WebBrowser,
  UnicodeString & Url, TNotifyEvent Handler)
{
  TBrowserViewer * BrowserViewer = dynamic_cast<TBrowserViewer *>(WebBrowser);
  if (DebugAlwaysTrue(BrowserViewer != NULL))
  {
    BrowserViewer->AddLinkHandler(Url, Handler);
  }
}

void NavigateBrowserToUrl(TWebBrowserEx * WebBrowser, UnicodeString & Url)
{
  TBrowserViewer * BrowserViewer = dynamic_cast<TBrowserViewer *>(WebBrowser);
  if (DebugAlwaysTrue(BrowserViewer != NULL))
  {
    BrowserViewer->NavigateToUrl(Url);
  }
}

void ReadyBrowserForStreaming(TWebBrowserEx * WebBrowser)
{
  // This creates TWebBrowserEx::Document, which we need to stream in an in-memory document
  NavigateBrowserToUrl(WebBrowser, L"about:blank");
  // Needs to be followed by WaitBrowserToIdle
}

void WaitBrowserToIdle(TWebBrowserEx * WebBrowser)
{
  while (WebBrowser->ReadyState < ::READYSTATE_INTERACTIVE)
  {
    Application->ProcessMessages();
  }
}

void HideBrowserScrollbars(TWebBrowserEx * WebBrowser)
{
  ICustomDoc * CustomDoc = NULL;
  if (DebugAlwaysTrue(WebBrowser->Document != NULL) &&
      SUCCEEDED(WebBrowser->Document->QueryInterface(&CustomDoc)) &&
      DebugAlwaysTrue(CustomDoc != NULL))
  {
    TCustomDocHandler * Handler = new TCustomDocHandler(WebBrowser);
    CustomDoc->SetUIHandler(Handler);
  }
}

UnicodeString GenerateAppHtmlPage(TFont * Font, TPanel * Parent, UnicodeString & Body, bool Seamless)
{
  UnicodeString Result =
    L"<!DOCTYPE html>\n"
    L"<meta charset=\"utf-8\">\n"
    L"<html>\n"
    L"<head>\n"
    L"<style>\n"
    L"\n"
    L"body\n"
    L"{\n"
    L"    font-family: '" + Font->Name + L"';\n"
    L"    margin: " + UnicodeString(Seamless ? L"0" : L"0.5em") + L";\n"
    L"    background-color: " + ColorToWebColorStr(Parent->Color) + L";\n" +
    UnicodeString(Seamless ? L"    overflow: hidden;\n" : L"") +
    L"}\n"
    L"\n"
    L"body\n"
    L"{\n"
    L"    font-size: " + IntToStr(Font->Size) + L"pt;\n"
    L"}\n"
    L"\n"
    L"p\n"
    L"{\n"
    L"    margin-top: 0;\n"
    L"    margin-bottom: 1em;\n"
    L"}\n"
    L"\n"
    L"a, a:visited, a:hover, a:visited, a:current\n"
    L"{\n"
    L"    color: " + ColorToWebColorStr(LinkColor) + L";\n"
    L"}\n"
    L"</style>\n"
    L"</head>\n"
    L"<body>\n" +
    Body +
    L"</body>\n"
    L"</html>\n";
  return Result;
}

void LoadBrowserDocument(TWebBrowserEx * WebBrowser, UnicodeString & Document)
{
  std::unique_ptr<TMemoryStream> DocumentStream(new TMemoryStream());
  UTF8String DocumentUTF8 = UTF8String(Document);
  DocumentStream->Write(DocumentUTF8.c_str(), DocumentUTF8.Length());
  DocumentStream->Seek(0, 0);

  // For stream-loaded document, when set only after loading from OnDocumentComplete,
  // browser stops working
  SetBrowserDesignModeOff(WebBrowser);

  TStreamAdapter * DocumentStreamAdapter = new TStreamAdapter(DocumentStream.get(), soReference);
  IPersistStreamInit * PersistStreamInit = NULL;
  if (DebugAlwaysTrue(WebBrowser->Document != NULL) &&
      SUCCEEDED(WebBrowser->Document->QueryInterface(IID_IPersistStreamInit, (void **)&PersistStreamInit)) &&
      DebugAlwaysTrue(PersistStreamInit != NULL))
  {
    PersistStreamInit->Load(static_cast<_di_IStream>(*DocumentStreamAdapter));
  }
}

{
  // This creates TWebBrowserEx::Document, which we need to stream in an in-memory document
  NavigateBrowserToUrl(WebBrowser, L"about:blank");
  // Needs to be followed by WaitBrowserToIdle
}

void WaitBrowserToIdle(TWebBrowserEx * WebBrowser)
{
  while (WebBrowser->ReadyState < ::READYSTATE_INTERACTIVE)
  {
    Application->ProcessMessages();
  }
}

void HideBrowserScrollbars(TWebBrowserEx * WebBrowser)
{
  ICustomDoc * CustomDoc = NULL;
  if (DebugAlwaysTrue(WebBrowser->Document != NULL) &&
      SUCCEEDED(WebBrowser->Document->QueryInterface(&CustomDoc)) &&
      DebugAlwaysTrue(CustomDoc != NULL))
  {
    TCustomDocHandler * Handler = new TCustomDocHandler(WebBrowser);
    CustomDoc->SetUIHandler(Handler);
  }
}

UnicodeString GenerateAppHtmlPage(TFont * Font, TPanel * Parent, UnicodeString & Body, bool Seamless)
{
  UnicodeString Result =
    L"<!DOCTYPE html>\n"
    L"<meta charset=\"utf-8\">\n"
    L"<html>\n"
    L"<head>\n"
    L"<style>\n"
    L"\n"
    L"body\n"
    L"{\n"
    L"    font-family: '" + Font->Name + L"';\n"
    L"    margin: " + UnicodeString(Seamless ? L"0" : L"0.5em") + L";\n"
    L"    background-color: " + ColorToWebColorStr(Parent->Color) + L";\n" +
    UnicodeString(Seamless ? L"    overflow: hidden;\n" : L"") +
    L"}\n"
    L"\n"
    L"body\n"
    L"{\n"
    L"    font-size: " + IntToStr(Font->Size) + L"pt;\n"
    L"}\n"
    L"\n"
    L"p\n"
    L"{\n"
    L"    margin-top: 0;\n"
    L"    margin-bottom: 1em;\n"
    L"}\n"
    L"\n"
    L"a, a:visited, a:hover, a:visited, a:current\n"
    L"{\n"
    L"    color: " + ColorToWebColorStr(LinkColor) + L";\n"
    L"}\n"
    L"</style>\n"
    L"</head>\n"
    L"<body>\n" +
    Body +
    L"</body>\n"
    L"</html>\n";
  return Result;
}

void LoadBrowserDocument(TWebBrowserEx * WebBrowser, UnicodeString & Document)
{
  std::unique_ptr<TMemoryStream> DocumentStream(new TMemoryStream());
  UTF8String DocumentUTF8 = UTF8String(Document);
  DocumentStream->Write(DocumentUTF8.c_str(), DocumentUTF8.Length());
  DocumentStream->Seek(0, 0);

  // For stream-loaded document, when set only after loading from OnDocumentComplete,
  // browser stops working
  SetBrowserDesignModeOff(WebBrowser);

  TStreamAdapter * DocumentStreamAdapter = new TStreamAdapter(DocumentStream.get(), soReference);
  IPersistStreamInit * PersistStreamInit = NULL;
  if (DebugAlwaysTrue(WebBrowser->Document != NULL) &&
      SUCCEEDED(WebBrowser->Document->QueryInterface(IID_IPersistStreamInit, (void **)&PersistStreamInit)) &&
      DebugAlwaysTrue(PersistStreamInit != NULL))
  {
    PersistStreamInit->Load(static_cast<_di_IStream>(*DocumentStreamAdapter));
  }
}

TComponent * FindComponentRecursively(TComponent * Root, UnicodeString & Name)
{
  for (int Index = 0; Index < Root->ComponentCount; Index++)
  {
    TComponent * Component = Root->Components[Index];
    if (CompareText(Component->Name, Name) == 0)
    {
      return Component;
    }

    Component = FindComponentRecursively(Component, Name);
    if (Component != NULL)
    {
      return Component;
    }
  }
  return NULL;
}

void GetInstrutionsTheme(
  TColor & MainInstructionColor, HFONT & MainInstructionFont, HFONT & InstructionFont)
{
  MainInstructionColor = Graphics::clNone;
  MainInstructionFont = 0;
  InstructionFont = 0;
  HTHEME Theme = OpenThemeData(0, L"TEXTSTYLE");
  if (Theme != NULL)
  {
    LOGFONT AFont;
    COLORREF AColor;

void GetInstrutionsTheme(
  TColor & MainInstructionColor, HFONT & MainInstructionFont, HFONT & InstructionFont)
{
  MainInstructionColor = Graphics::clNone;
  MainInstructionFont = 0;
  InstructionFont = 0;
  HTHEME Theme = OpenThemeData(0, L"TEXTSTYLE");
  if (Theme != NULL)
  {
    LOGFONT AFont;
    COLORREF AColor;

    memset(&AFont, 0, sizeof(AFont));
    // Using Canvas->Handle in the 2nd argument we can get scaled font,
    // but at this point the form is sometime not scaled yet (difference is particularly for standalone messages like
    // /UninstallCleanup), so the results are inconsistent.
    if (GetThemeFont(Theme, NULL, TEXT_MAININSTRUCTION, 0, TMT_FONT, &AFont) == S_OK)
    {
      MainInstructionFont = CreateFontIndirect(&AFont);
    }
    if (GetThemeColor(Theme, TEXT_MAININSTRUCTION, 0, TMT_TEXTCOLOR, &AColor) == S_OK)
    {
      MainInstructionColor = (TColor)AColor;
    }

    memset(&AFont, 0, sizeof(AFont));
    if (GetThemeFont(Theme, NULL, TEXT_INSTRUCTION, 0, TMT_FONT, &AFont) == S_OK)
    {
      InstructionFont = CreateFontIndirect(&AFont);
    }

    CloseThemeData(Theme);
  }
}
#endif // #if 0

TLocalCustomCommand::TLocalCustomCommand() noexcept
{
}

TLocalCustomCommand::TLocalCustomCommand(
  const TCustomCommandData &Data, UnicodeString RemotePath, UnicodeString LocalPath) noexcept :
  TFileCustomCommand(Data, RemotePath)
{
  FLocalPath = LocalPath;
}

TLocalCustomCommand::TLocalCustomCommand(const TCustomCommandData &Data,
  UnicodeString RemotePath, UnicodeString LocalPath, UnicodeString FileName,
  UnicodeString LocalFileName, UnicodeString FileList) noexcept :
  TFileCustomCommand(Data, RemotePath, FileName, FileList)
{
  FLocalPath = LocalPath;
  FLocalFileName = LocalFileName;
}

intptr_t TLocalCustomCommand::PatternLen(UnicodeString Command, intptr_t Index) const
{
  intptr_t Len;
  if ((Index < Command.Length()) && (Command[Index + 1] == L'\\'))
  {
    Len = 2;
  }
  else if ((Index < Command.Length()) && (Command[Index + 1] == L'^'))
  {
    Len = 3;
  }
  else
  {
    Len = TFileCustomCommand::PatternLen(Command, Index);
  }
  return Len;
}

bool TLocalCustomCommand::PatternReplacement(
  intptr_t Index, UnicodeString Pattern, UnicodeString &Replacement, bool &Delimit) const
{
  bool Result;
  if (Pattern == L"!\\")
  {
    // When used as "!\" in an argument to PowerShell, the trailing \ would escape the ",
    // so we exclude it
    Replacement = ::ExcludeTrailingBackslash(FLocalPath);
    Result = true;
  }
  else if (Pattern == L"!^!")
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

void TLocalCustomCommand::DelimitReplacement(
  UnicodeString & /*Replacement*/, wchar_t /*Quote*/)
{
  // never delimit local commands
}

bool TLocalCustomCommand::HasLocalFileName(UnicodeString Command) const
{
  return FindPattern(Command, L'^');
}

bool TLocalCustomCommand::IsFileCommand(UnicodeString Command) const
{
  return TFileCustomCommand::IsFileCommand(Command) || HasLocalFileName(Command);
}


#if 0
typedef std::set<TDataModule *> TImagesModules;
static TImagesModules ImagesModules;
static std::map<int, TPngImageList *> AnimationsImages;
static std::map<int, TImageList *> ButtonImages;
static std::map<int, TPngImageList *> DialogImages;

int NormalizePixelsPerInch(int PixelsPerInch)
{
  if (PixelsPerInch >= 192)
  {
    PixelsPerInch = 192;
  }
  else if (PixelsPerInch >= 144)
  {
    PixelsPerInch = 144;
  }
  else if (PixelsPerInch >= 120)
  {
    PixelsPerInch = 120;
  }
  else
  {
    PixelsPerInch = 96;
  }
  return PixelsPerInch;
}

static int NeedImagesModule(TControl * Control)
{
  int PixelsPerInch = NormalizePixelsPerInch(GetControlPixelsPerInch(Control));

  if (AnimationsImages.find(PixelsPerInch) == AnimationsImages.end())
  {
    TDataModule * ImagesModule;
    HANDLE ResourceModule = GUIConfiguration->ChangeToDefaultResourceModule();
    try
    {
      if (PixelsPerInch == 192)
      {
        ImagesModule = new TAnimations192Module(Application);
      }
      else if (PixelsPerInch == 144)
      {
        ImagesModule = new TAnimations144Module(Application);
      }
      else if (PixelsPerInch == 120)
      {
        ImagesModule = new TAnimations120Module(Application);
      }
      else
      {
        DebugAssert(PixelsPerInch == 96);
        ImagesModule = new TAnimations96Module(Application);
      }

      ImagesModules.insert(ImagesModule);

      TPngImageList * AAnimationImages =
        DebugNotNull(dynamic_cast<TPngImageList *>(ImagesModule->FindComponent(L"AnimationImages")));
      AnimationsImages.insert(std::make_pair(PixelsPerInch, AAnimationImages));

      TImageList * AButtonImages =
        DebugNotNull(dynamic_cast<TImageList *>(ImagesModule->FindComponent(L"ButtonImages")));
      ButtonImages.insert(std::make_pair(PixelsPerInch, AButtonImages));

      TPngImageList * ADialogImages =
        DebugNotNull(dynamic_cast<TPngImageList *>(ImagesModule->FindComponent(L"DialogImages")));
      DialogImages.insert(std::make_pair(PixelsPerInch, ADialogImages));
    }
    __finally
    {
      GUIConfiguration->ChangeResourceModule(ResourceModule);
    }
  }

  return PixelsPerInch;
}

TPngImageList * GetAnimationsImages(TControl * Control)
{
  int PixelsPerInch = NeedImagesModule(Control);
  return DebugNotNull(AnimationsImages[PixelsPerInch]);
}

TImageList * GetButtonImages(TControl * Control)
{
  int PixelsPerInch = NeedImagesModule(Control);
  return DebugNotNull(ButtonImages[PixelsPerInch]);
}

TPngImageList * GetDialogImages(TControl * Control)
{
  int PixelsPerInch = NeedImagesModule(Control);
  return DebugNotNull(DialogImages[PixelsPerInch]);
}

void ReleaseImagesModules()
{

  TImagesModules::iterator i = ImagesModules.begin();
  while (i != ImagesModules.end())
  {
    delete (*i);
    i++;
  }
  ImagesModules.clear();
}

TFrameAnimation::TFrameAnimation()
{
  FFirstFrame = -1;
}

void TFrameAnimation::Init(TPaintBox * PaintBox, UnicodeString & Name)
{
  FName = Name;
  FPaintBox = PaintBox;

  FPaintBox->ControlStyle = FPaintBox->ControlStyle << csOpaque;
  DebugAssert((FPaintBox->OnPaint == NULL) || (FPaintBox->OnPaint == PaintBoxPaint));
  FPaintBox->OnPaint = PaintBoxPaint;
  SetRescaleFunction(FPaintBox, PaintBoxRescale, reinterpret_cast<TObject *>(this));

  DoInit();
}

void TFrameAnimation::DoInit()
{
  FImageList = GetAnimationsImages(FPaintBox);
  FFirstFrame = -1;
  FFirstLoopFrame = -1;
  FPaintBox->Width = FImageList->Width;
  FPaintBox->Height = FImageList->Height;

  if (!FName.IsEmpty())
  {
    int Frame = 0;
    while (Frame < FImageList->PngImages->Count)
    {
      UnicodeString FrameData = FImageList->PngImages->Items[Frame]->Name;
      UnicodeString FrameName;
      FrameName = CutToChar(FrameData, L'_', false);

      if (SameText(FName, FrameName))
      {
        int FrameIndex = StrToInt(CutToChar(FrameData, L'_', false));
        if (FFirstFrame < 0)
        {
          FFirstFrame = Frame;
        }
        if ((FFirstLoopFrame < 0) && (FrameIndex > 0))
        {
          FFirstLoopFrame = Frame;
        }
        FLastFrame = Frame;
      }
      else
      {
        if (FFirstFrame >= 0)
        {
          // optimization
          break;
        }
      }
      Frame++;
    }

    DebugAssert(FFirstFrame >= 0);
    DebugAssert(FFirstLoopFrame >= 0);
  }

  Stop();
}

void TFrameAnimation::PaintBoxRescale(TComponent * /*Sender*/, TObject * Token)
{
  TFrameAnimation * FrameAnimation = reinterpret_cast<TFrameAnimation *>(Token);
  FrameAnimation->Rescale();
}

void TFrameAnimation::Rescale()
{
  bool Started = (FTimer != NULL) && FTimer->Enabled;
  DoInit();
  if (Started)
  {
    Start();
  }
}

void TFrameAnimation::Start()
{
  if (FFirstFrame >= 0)
  {
    FNextFrameTick = GetTickCount();
    CalculateNextFrameTick();

    if (FTimer == NULL)
    {
      FTimer = new TTimer(GetParentForm(FPaintBox));
      FTimer->Interval = static_cast<int>(GUIUpdateInterval);
      FTimer->OnTimer = Timer;
    }
    else
    {
      // reset timer
      FTimer->Enabled = false;
      FTimer->Enabled = true;
    }
  }
}

void TFrameAnimation::Timer(TObject * /*Sender*/)
{
  Animate();
}

void TFrameAnimation::PaintBoxPaint(TObject * Sender)
{
  if (FFirstFrame >= 0)
  {
    // Double-buffered drawing to prevent flicker (as the images are transparent)
    DebugUsedParam(Sender);
    DebugAssert(FPaintBox == Sender);
    DebugAssert(FPaintBox->ControlStyle.Contains(csOpaque));
    std::unique_ptr<TBitmap> Bitmap(new TBitmap());
    Bitmap->SetSize(FPaintBox->Width, FPaintBox->Height);
    Bitmap->Canvas->Brush->Color = FPaintBox->Color;
    TRect Rect(0, 0, Bitmap->Width, Bitmap->Height);
    Bitmap->Canvas->FillRect(Rect);
    TGraphic * Graphic = GetCurrentImage()->PngImage;
    DebugAssert(Graphic->Width == FPaintBox->Width);
    DebugAssert(Graphic->Height == FPaintBox->Height);
    Bitmap->Canvas->Draw(0, 0, Graphic);
    FPaintBox->Canvas->Draw(0, 0, Bitmap.get());
  }
  FPainted = true;
}

void TFrameAnimation::Repaint()
{
  FPainted = false;
  // If the form is not showing yet, the Paint() is not even called
  FPaintBox->Repaint();
  if (!FPainted)
  {
    // Paint later, alternativelly we may keep trying Repaint() in Animate().
    // See also a hack in TAuthenticateForm::Log.
    FPaintBox->Invalidate();
  }
}

void TFrameAnimation::Stop()
{
  FNextFrameTick = std::numeric_limits<DWORD>::max();
  FCurrentFrame = FFirstFrame;
  Repaint();

  if (FTimer != NULL)
  {
    FTimer->Enabled = false;
  }
}

void TFrameAnimation::Animate()
{
  if (FFirstFrame >= 0)
  {
    // UPGRADE: Use GetTickCount64() when we stop supporting Windows XP.
    DWORD TickCount = GetTickCount();

    // Keep in sync with an opposite condition at the end of the loop.
    // We may skip some frames if we got stalled for a while
    while (TickCount >= FNextFrameTick)
    {
      if (FCurrentFrame >= FLastFrame)
      {
        FCurrentFrame = FFirstLoopFrame;
      }
      else
      {
        FCurrentFrame++;
      }

      CalculateNextFrameTick();

      Repaint();
    }
  }
}

TPngImageCollectionItem * TFrameAnimation::GetCurrentImage()
{
  return FImageList->PngImages->Items[FCurrentFrame];
}

void TFrameAnimation::CalculateNextFrameTick()
{
  TPngImageCollectionItem * ImageItem = GetCurrentImage();
  UnicodeString Duration = ImageItem->Name;
  CutToChar(Duration, L'_', false);
  // skip index (is not really used)
  CutToChar(Duration, L'_', false);
  // This should overflow, when tick count wraps.
  FNextFrameTick += StrToInt(Duration) * 10;
}


// Hints use:
// - Cleanup list tooltip (multi line)
// - Combo edit button
// - Transfer settings label (multi line, follows label size and font)
// - HintLabel (hint and persistent hint, multi line)
// - status bar hints

TScreenTipHintWindow::TScreenTipHintWindow(TComponent * Owner) :
  THintWindow(Owner)
{
  FParentPainting = false;
}

int TScreenTipHintWindow::GetTextFlags(TControl * Control)
{
  return DT_LEFT | DT_WORDBREAK | DT_NOPREFIX | Control->DrawTextBiDiModeFlagsReadingOnly();
}

bool TScreenTipHintWindow::UseBoldShortHint(TControl * HintControl)
{
  return
    (dynamic_cast<TTBCustomDockableWindow *>(HintControl) != NULL) ||
    (dynamic_cast<TTBPopupWindow *>(HintControl) != NULL) ||
    (HintControl->Perform(WM_WANTS_SCREEN_TIPS, 0, 0) == 1);
}

bool TScreenTipHintWindow::IsPathLabel(TControl * HintControl)
{
  return (dynamic_cast<TPathLabel *>(HintControl) != NULL);
}

intptr_t TScreenTipHintWindow::GetMargin(TControl *HintControl, UnicodeString Hint)
{
  int Result;

  if (HasLabelHintPopup(HintControl, Hint) || IsPathLabel(HintControl))
  {
    Result = 3;
  }
  else
  {
    Result = 6;
  }

  Result = ScaleByTextHeight(HintControl, Result);

  return Result;
}

TFont * TScreenTipHintWindow::GetFont(TControl * HintControl, UnicodeString & Hint)
{
  TFont * Result;
  if (HasLabelHintPopup(HintControl, Hint) || IsPathLabel(HintControl))
  {
    Result = reinterpret_cast<TLabel *>(dynamic_cast<TCustomLabel *>(HintControl))->Font;
  }
  else
  {
    FScaledHintFont.reset(new TFont());
    FScaledHintFont->Assign(Screen->HintFont);
    FScaledHintFont->Size = ScaleByPixelsPerInchFromSystem(FScaledHintFont->Size, HintControl);
    Result = FScaledHintFont.get();
  }
  return Result;
}

void TScreenTipHintWindow::CalcHintTextRect(TControl * Control, TCanvas * Canvas, TRect & Rect, UnicodeString & Hint)
{
  const int Flags = DT_CALCRECT | GetTextFlags(Control);
  DrawText(Canvas->Handle, Hint.c_str(), -1, &Rect, Flags);
}

TRect TScreenTipHintWindow::CalcHintRect(int MaxWidth, UnicodeString AHint, void * AData)
{
  TControl * HintControl = GetHintControl(AData);
  int Margin = GetMargin(HintControl, AHint);
  UnicodeString ShortHint;
  UnicodeString LongHint;
  SplitHint(HintControl, AHint, ShortHint, LongHint);

  Canvas->Font->Assign(GetFont(HintControl, AHint));

  const int ScreenTipTextOnlyWidth = ScaleByTextHeight(HintControl, cScreenTipTextOnlyWidth);

  int LongHintMargin = 0; // shut up
  bool HasLongHint = !LongHint.IsEmpty();
  if (HasLongHint)
  {
    // double-margin on the right
    LongHintMargin = (3 * Margin);
    MaxWidth = ScreenTipTextOnlyWidth - LongHintMargin;
  }

  // Multi line short hints can be twice as wide, to not break the individual lines unless really necessary.
  // (login site tree, clean up dialog list, preferences custom command list, persistent hint, etc).
  // And they also can be twice as wide, to not break the individual lines unless really necessary.
  if (ShortHint.Pos(L"\n") > 0)
  {
    MaxWidth *= 2;
  }

  bool HintPopup = HasLabelHintPopup(HintControl, AHint);
  if (HintPopup)
  {
    MaxWidth = HintControl->Width;
  }

  if (UseBoldShortHint(HintControl))
  {
    Canvas->Font->Style = TFontStyles() << fsBold;
  }
  TRect ShortRect(0, 0, MaxWidth, 0);
  CalcHintTextRect(this, Canvas, ShortRect, ShortHint);
  Canvas->Font->Style = TFontStyles();

  TRect Result;

  if (!HasLongHint)
  {
    Result = ShortRect;

    if (HintPopup)
    {
      Result.Right = MaxWidth + 2 * Margin;
    }
    else
    {
      Result.Right += 3 * Margin;
    }

    Result.Bottom += 2 * Margin;
  }
  else
  {
    const int LongIndentation = Margin * 3 / 2;
    TRect LongRect(0, 0, MaxWidth - LongIndentation, 0);
    CalcHintTextRect(this, Canvas, LongRect, LongHint);

    Result.Right = Max(ScreenTipTextOnlyWidth, LongRect.Right + LongIndentation + LongHintMargin);
    Result.Bottom = Margin + ShortRect.Height() + (Margin / 3 * 5) + LongRect.Height() + Margin;
  }

  // VCLCOPY: To counter the increase in THintWindow::ActivateHintData
  Result.Bottom -= 4;

  return Result;
}

void TScreenTipHintWindow::SplitHint(
  TControl * HintControl, UnicodeString & Hint, UnicodeString & ShortHint, UnicodeString & LongHint)
{
  if (HasLabelHintPopup(HintControl, Hint))
  {
    ShortHint = HintControl->Hint;
  }
  else
  {
    ShortHint = GetShortHint(Hint);
    LongHint = GetLongHintIfAny(Hint);
  }
}

void TScreenTipHintWindow::ActivateHintData(const TRect & ARect, UnicodeString AHint, void * AData)
{
  FHintControl = GetHintControl(AData);
  SplitHint(FHintControl, AHint, FShortHint, FLongHint);
  FMargin = GetMargin(FHintControl, AHint);
  FHintPopup = HasLabelHintPopup(FHintControl, AHint);

  Canvas->Font->Assign(GetFont(FHintControl, AHint));

  TRect Rect = ARect;

  if (FHintPopup)
  {
    Rect.SetLocation(FHintControl->ClientToScreen(TPoint(-FMargin, -FMargin)));
  }
  if (IsPathLabel(FHintControl))
  {
    Rect.Offset(-FMargin, -FMargin);
  }

  THintWindow::ActivateHintData(Rect, FShortHint, AData);
}

TControl * TScreenTipHintWindow::GetHintControl(void * Data)
{
  return reinterpret_cast<TControl *>(DebugNotNull(Data));
}

UnicodeString TScreenTipHintWindow::GetLongHintIfAny(UnicodeString & AHint)
{
  UnicodeString Result;
  int P = Pos(L"|", AHint);
  if (P > 0)
  {
    Result = GetLongHint(AHint);
  }
  return Result;
}

void TScreenTipHintWindow::Dispatch(void * AMessage)
{
  TMessage * Message = static_cast<TMessage*>(AMessage);
  switch (Message->Msg)
  {
    case WM_GETTEXTLENGTH:
      if (FParentPainting)
      {
        // make THintWindow::Paint() not paint the Caption
        Message->Result = 0;
      }
      else
      {
        THintWindow::Dispatch(AMessage);
      }
      break;

    default:
      THintWindow::Dispatch(AMessage);
      break;
  }
}

void TScreenTipHintWindow::Paint()
{
  // paint frame/background
  {
    TAutoFlag ParentPaintingFlag(FParentPainting);
    THintWindow::Paint();
  }

  const int Flags = GetTextFlags(this);
  const int Margin = FMargin - 1; // 1 = border

  TRect Rect = ClientRect;
  Rect.Inflate(-Margin, -Margin);
  if (!FHintPopup)
  {
    Rect.Right -= FMargin;
  }
  if (UseBoldShortHint(FHintControl))
  {
    Canvas->Font->Style = TFontStyles() << fsBold;
  }
  DrawText(Canvas->Handle, FShortHint.c_str(), -1, &Rect, Flags);
  TRect ShortRect = Rect;
  DrawText(Canvas->Handle, FShortHint.c_str(), -1, &ShortRect, DT_CALCRECT | Flags);
  Canvas->Font->Style = TFontStyles();

  if (!FLongHint.IsEmpty())
  {
    Rect.Left += FMargin * 3 / 2;
    Rect.Top += ShortRect.Height() + (FMargin / 3 * 5);
    DrawText(Canvas->Handle, FLongHint.c_str(), -1, &Rect, Flags);
  }
}


TNewRichEdit::TNewRichEdit(TComponent * AOwner) :
  TRichEdit(AOwner),
  FLibrary(0)
{
}

void TNewRichEdit::CreateParams(TCreateParams & Params)
{
  UnicodeString RichEditModuleName(L"MSFTEDIT.DLL");
  long int OldError;

  OldError = SetErrorMode(SEM_NOOPENFILEERRORBOX);
  FLibrary = LoadLibrary(RichEditModuleName.c_str());
  SetErrorMode(OldError);

  // No fallback, MSFTEDIT.DLL is available since Windows XP
  // https://learn.microsoft.com/en-us/archive/blogs/murrays/richedit-versions
  if (FLibrary == 0)
  {
    throw Exception(FORMAT(L"Cannot load %s", (RichEditModuleName)));
  }

  TCustomMemo::CreateParams(Params);
  // MSDN says that we should use MSFTEDIT_CLASS to load Rich Edit 4.1:
  // https://learn.microsoft.com/en-us/windows/win32/controls/about-rich-edit-controls
  // But MSFTEDIT_CLASS is defined as "RICHEDIT50W",
  // so not sure what version we are loading.
  // Seem to work on Windows XP SP3.
  CreateSubClass(Params, MSFTEDIT_CLASS);
}

void TNewRichEdit::CreateWnd()
{
  TRichEdit::CreateWnd();
  SendMessage(Handle, EM_SETEDITSTYLEEX, 0, SES_EX_HANDLEFRIENDLYURL);
}

void TNewRichEdit::DestroyWnd()
{
  TRichEdit::DestroyWnd();

  if (FLibrary != 0)
  {
    FreeLibrary(FLibrary);
  }
}
#endif // #if 0

UnicodeString ItemsFormatString(UnicodeString SingleItemFormat,
  UnicodeString MultiItemsFormat, intptr_t Count, UnicodeString FirstItem)
{
  UnicodeString Result;
  if (Count == 1)
  {
    Result = FORMAT(SingleItemFormat, FirstItem);
  }
  else
  {
    Result = FORMAT(MultiItemsFormat, Count);
  }
  return Result;
}

UnicodeString ItemsFormatString(UnicodeString SingleItemFormat,
  UnicodeString MultiItemsFormat, const TStrings *Items)
{
  return ItemsFormatString(SingleItemFormat, MultiItemsFormat,
      Items->GetCount(), (Items->GetCount() > 0 ? Items->GetString(0) : UnicodeString()));
}

UnicodeString FileNameFormatString(UnicodeString SingleFileFormat,
  UnicodeString MultiFilesFormat, const TStrings *AFiles, bool Remote)
{
  DebugAssert(AFiles != nullptr);
  UnicodeString Item;
  if (AFiles && (AFiles->GetCount() > 0))
  {
    Item = Remote ? base::UnixExtractFileName(AFiles->GetString(0)) :
      base::ExtractFileName(AFiles->GetString(0), true);
  }
  return ItemsFormatString(SingleFileFormat, MultiFilesFormat,
      AFiles ? AFiles->GetCount() : 0, Item);
}
#if 0

static int HideAccelFlag(TControl * Control)
{
  //ask the top level window about its UI state
  while (Control->Parent != NULL)
  {
    Control = Control->Parent;
  }
  int Result;
  if (FLAGSET(Control->Perform(WM_QUERYUISTATE, 0, 0), UISF_HIDEACCEL))
  {
    Result = DT_HIDEPREFIX;
  }
  else
  {
    Result = 0;
  }
  return Result;
}

void TUIStateAwareLabel::DoDrawText(TRect & Rect, int Flags)
{
  if (ShowAccelChar)
  {
    Flags = Flags | HideAccelFlag(this);
  }
  TLabel::DoDrawText(Rect, Flags);
}

void FindComponentClass(
  void *, TReader *, UnicodeString DebugUsedArg(ClassName), TComponentClass & ComponentClass)
{
  if (ComponentClass == __classid(TLabel))
  {
    ComponentClass = __classid(TUIStateAwareLabel);
  }
  else if (ComponentClass == __classid(TComboBox))
  {
    ComponentClass = __classid(TUIStateAwareComboBox);
  }
}

bool CanShowTimeEstimate(TDateTime StartTime)
{
  return (SecondsBetween(StartTime, Now()) >= 3);
}
class TSystemRequiredThread : public TSignalThread
{
public:
  TSystemRequiredThread();
  void Required();
protected:
  virtual bool WaitForEvent();
  virtual void ProcessEvent();
private:
  bool FRequired;
  TDateTime FLastRequired;
};

std::unique_ptr<TCriticalSection> SystemRequiredThreadSection(TraceInitPtr(new TCriticalSection()));
TSystemRequiredThread * SystemRequiredThread = NULL;

TSystemRequiredThread::TSystemRequiredThread() :
  TSignalThread(true), FRequired(false)
{
}

void TSystemRequiredThread::Required()
{
  // guarded in SystemRequired()
  FLastRequired = Now();
  TriggerEvent();
}

bool TSystemRequiredThread::WaitForEvent()
{
  const int ExpireInterval = 5000;
  bool Result = (TSignalThread::WaitForEvent(ExpireInterval) > 0);
  if (!Result && !FTerminated)
  {
    TGuard Guard(SystemRequiredThreadSection.get());
    if (!FTerminated && FRequired &&
        (MilliSecondsBetween(Now(), FLastRequired) > ExpireInterval))
    {
      AppLog("System is not required");
      SetThreadExecutionState(ES_CONTINUOUS);
      FLastRequired = TDateTime();
      FRequired = false;
    }
  }
  return Result;
}

void TSystemRequiredThread::ProcessEvent()
{
  TGuard Guard(SystemRequiredThreadSection.get());
  if (!FRequired &&
      (FLastRequired != TDateTime()))
  {
    AppLog("System is required");
    SetThreadExecutionState(ES_SYSTEM_REQUIRED | ES_CONTINUOUS);
    FRequired = true;
  }
}

void SystemRequired()
{
  if (IsWin11())
  {
    TGuard Guard(SystemRequiredThreadSection.get());
    if (SystemRequiredThread == NULL)
    {
      AppLog("Starting system required thread");
      SystemRequiredThread = new TSystemRequiredThread();
      SystemRequiredThread->Start();
    }
    SystemRequiredThread->Required();
  }
  else
  {
    SetThreadExecutionState(ES_SYSTEM_REQUIRED);
  }
}

void GUIFinalize()
{
  TSystemRequiredThread * Thread;
  {
    TGuard Guard(SystemRequiredThreadSection.get());
    Thread = SystemRequiredThread;
    SystemRequiredThread = NULL;
  }

  if (Thread != NULL)
  {
    AppLog("Stopping system required thread");
    Thread->Terminate();
    Thread->WaitFor();
    delete Thread;
  }
}
#endif // #if 0
