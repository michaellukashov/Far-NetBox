
#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <TextsCore.h>
#include <SessionData.h>
#include <CoreMain.h>
#include <Interface.h>
#include "CustomWinConfiguration.h"
#include <Exceptions.h>
#include <PasTools.hpp>
#include <Math.hpp>

#pragma package(smart_init)

TCustomWinConfiguration * CustomWinConfiguration = nullptr;

class THistoryStrings : public TStringList
{
public:
  THistoryStrings() : TStringList()
  {
    FModified = false;
  };

  __property bool Modified = { read = FModified, write = FModified };

private:
  bool FModified;
};

TCustomWinConfiguration::TCustomWinConfiguration():
  TGUIConfiguration()
{
  FHistory = new TStringList();
  FEmptyHistory = new TStringList();
  FDefaultInterface = ifCommander;
  FCanApplyInterfaceImmediately = true;
}

TCustomWinConfiguration::~TCustomWinConfiguration()
{
  ClearHistory();
  delete FHistory;
  delete FEmptyHistory;
}

void TCustomWinConfiguration::ClearHistory()
{
  DebugAssert(FHistory != nullptr);

  THistoryStrings * HistoryStrings;
  for (int Index = 0; Index < FHistory->Count; Index++)
  {
    HistoryStrings = dynamic_cast<THistoryStrings *>(FHistory->Objects[Index]);
    FHistory->Objects[Index] = nullptr;
    SAFE_DESTROY(HistoryStrings);
  }
  FHistory->Clear();
}

void TCustomWinConfiguration::DefaultHistory()
{
  ClearHistory();

  std::unique_ptr<THistoryStrings> Strings;

  // Defaults for speed limits.
  Strings = std::make_unique<THistoryStrings>();
  // This is language-specifics, what has to be dealt with when changing language.
  // There's ad-hoc workaround in CopySpeedLimits.
  // If we need to solve this for another history, we should introduce
  // a generic solution, like language-specific history ("SpeedLimitEN")
  Strings->Add(LoadStr(SPEED_UNLIMITED));
  unsigned long Speed = MaxSpeed;
  while (Speed >= MinSpeed)
  {
    Strings->Add(SetSpeedLimit(Speed));
    Speed = Speed / 2;
  }
  FHistory->AddObject(L"SpeedLimit", Strings.release());

  Strings = std::make_unique<THistoryStrings>();
  Strings->Add(FormatCommand(DefaultPuttyPath, L""));
  Strings->Add(FormatCommand(DefaultPuttyPath, L"-t -m \"%TEMP%\\putty.txt\" !`cmd.exe /c echo cd '!/' ; /bin/bash -login > \"%TEMP%\\putty.txt\"`"));
  Strings->Add(KittyExecutable);
  Strings->Add(FORMAT(L"%s -cmd \"cd '!/'\" !U@!@ -P !# -title \"!N\"", KittyExecutable));
  Strings->Add(L"%SystemRoot%\\Sysnative\\OpenSSH\\ssh.exe !U@!@ -p !#");
  Strings->Add(L"%SystemRoot%\\Sysnative\\OpenSSH\\ssh.exe !U@!@ -p !# -t \"cd !/ ; /bin/bash\"");
  FHistory->AddObject(L"PuttyPath", Strings.release());
}

UnicodeString TCustomWinConfiguration::FormatDefaultWindowSize(int Width, int Height)
{
  return FORMAT(L"%d,%d,%s", (Width, Height, SaveDefaultPixelsPerInch()));
}

UnicodeString TCustomWinConfiguration::FormatDefaultWindowParams(int Width, int Height)
{
  return FORMAT(L"-1;-1;%d;%d;%d;%s", (Width, Height, int(wsNormal), SaveDefaultPixelsPerInch()));
}

void TCustomWinConfiguration::Default()
{
  TGUIConfiguration::Default();

  FInterface = FDefaultInterface;
  int WorkAreaWidthScaled = DimensionToDefaultPixelsPerInch(Screen->WorkAreaWidth);
  int WorkAreaHeightScaled = DimensionToDefaultPixelsPerInch(Screen->WorkAreaHeight);
  // Same as commander interface (really used only with /synchronize)
  int ChecklistWidth = Min(WorkAreaWidthScaled - 40, 1090);
  int ChecklistHeight = Min(WorkAreaHeightScaled - 30, 700);
  // 0 means no "custom-pos"
  FSynchronizeChecklist.WindowParams = L"0;" + FormatDefaultWindowParams(ChecklistWidth, ChecklistHeight);
  FSynchronizeChecklist.ListParams = L"1;1|150,1;100,1;80,1;130,1;25,1;100,1;80,1;130,1;@" + SaveDefaultPixelsPerInch() + L"|0;1;2;3;4;5;6;7";
  FFindFile.WindowParams = FormatDefaultWindowSize(646, 481);
  FFindFile.ListParams = L"1;1|125,1;181,1;80,1;122,1;@" + SaveDefaultPixelsPerInch() + L"|0;1;2;3|/1";
  FConsoleWin.WindowSize = FormatDefaultWindowSize(570, 430);
  FLoginDialog.WindowSize = FormatDefaultWindowSize(640, 430);
  FLoginDialog.SiteSearch = isName;
  FConfirmExitOnCompletion = true;
  FSynchronizeSummary = true;
  FSessionColors = L"";
  FFontColors = L"";
  FCopyShortCutHintShown = false;
  FHttpForWebDAV = false;
  FDefaultFixedWidthFontName = L"";
  FDefaultFixedWidthFontSize = 0;

  DefaultHistory();
}

void TCustomWinConfiguration::Saved()
{
  TGUIConfiguration::Saved();

  THistoryStrings * HistoryStrings;
  for (int Index = 0; Index < FHistory->Count; Index++)
  {
    HistoryStrings = dynamic_cast<THistoryStrings *>(FHistory->Objects[Index]);
    DebugAssert(HistoryStrings != nullptr);
    HistoryStrings->Modified = false;
  }
}

// duplicated from core\configuration.cpp
#undef LASTELEM
#define LASTELEM(ELEM) \
  ELEM.SubString(ELEM.LastDelimiter(L".>")+1, ELEM.Length() - ELEM.LastDelimiter(L".>"))
#define BLOCK(KEY, CANCREATE, BLOCK) \
  do { if (Storage->OpenSubKeyPath(KEY, CANCREATE)) try { BLOCK } __finally { Storage->CloseSubKeyPath(); } } while(0)
#define REGCONFIG(CANCREATE) \
  BLOCK("Interface", CANCREATE, \
    KEY(Integer,  Interface); \
    KEY(Bool,     ConfirmExitOnCompletion); \
    KEY(Bool,     SynchronizeSummary); \
    KEY(String,   SessionColors); \
    KEY(String,   FontColors); \
    KEY(Bool,     CopyShortCutHintShown); \
    KEY(Bool,     HttpForWebDAV); \
  ) \
  BLOCK("Interface\\SynchronizeChecklist", CANCREATE, \
    KEY(String,   SynchronizeChecklist.WindowParams); \
    KEY(String,   SynchronizeChecklist.ListParams); \
  ); \
  BLOCK("Interface\\FindFile", CANCREATE, \
    KEY(String,   FindFile.WindowParams); \
    KEY(String,   FindFile.ListParams); \
  ); \
  BLOCK("Interface\\ConsoleWin", CANCREATE, \
    KEY(String,   ConsoleWin.WindowSize); \
  ); \
  BLOCK("Interface\\LoginDialog", CANCREATE, \
    KEY(String,   LoginDialog.WindowSize); \
    KEY(Integer,  LoginDialog.SiteSearch); \
  ); \

void TCustomWinConfiguration::SaveData(
  THierarchicalStorage * Storage, bool All)
{
  TGUIConfiguration::SaveData(Storage, All);

  // duplicated from core\configuration.cpp
  #define KEY(TYPE, VAR) Storage->Write ## TYPE(LASTELEM(UnicodeString(TEXT(#VAR))), VAR)
  #define KEYEX(TYPE, VAR, NAME) Storage->Write ## TYPE(NAME, VAR)
  #pragma warn -eas
  REGCONFIG(true);
  #pragma warn +eas
  #undef KEYEX
  #undef KEY

  if (FHistory->Count > 0)
  {
    if (Storage->OpenSubKey(L"History", true))
    {
      try
      {
        THistoryStrings * HistoryStrings;
        for (int Index = 0; Index < FHistory->Count; Index++)
        {
          HistoryStrings = dynamic_cast<THistoryStrings *>(FHistory->Objects[Index]);
          DebugAssert(HistoryStrings != nullptr);
          if (All || HistoryStrings->Modified)
          {
            if (Storage->OpenSubKey(FHistory->Strings[Index], true))
            {
              try
              {
                Storage->WriteValues(HistoryStrings);
              }
              __finally
              {
                Storage->CloseSubKey();
              }
            }
          }
        }
      }
      __finally
      {
        Storage->CloseSubKey();
      }
    }

    if (Storage->OpenSubKey(L"HistoryParams", true))
    {
      try
      {
        THistoryStrings * HistoryStrings;
        for (int Index = 0; Index < FHistory->Count; Index++)
        {
          HistoryStrings = dynamic_cast<THistoryStrings *>(FHistory->Objects[Index]);
          DebugAssert(HistoryStrings != nullptr);
          if (All || HistoryStrings->Modified)
          {
            bool HasData = false;
            for (int VIndex = 0; !HasData && (VIndex < HistoryStrings->Count); VIndex++)
            {
              HasData = (HistoryStrings->Objects[VIndex] != nullptr);
            }

            if (!HasData)
            {
              Storage->RecursiveDeleteSubKey(FHistory->Strings[Index]);
            }
            else if (Storage->OpenSubKey(FHistory->Strings[Index], true))
            {
              try
              {
                Storage->ClearValues();
                for (int VIndex = 0; VIndex < HistoryStrings->Count; VIndex++)
                {
                  void * Data = HistoryStrings->Objects[VIndex];
                  Storage->WriteBinaryData(IntToStr(VIndex), &Data, sizeof(Data));
                }
              }
              __finally
              {
                Storage->CloseSubKey();
              }
            }
          }
        }
      }
      __finally
      {
        Storage->CloseSubKey();
      }
    }
  }
}

void TCustomWinConfiguration::LoadData(
  THierarchicalStorage * Storage)
{
  TGUIConfiguration::LoadData(Storage);

  FAppliedInterface = FInterface;

  // duplicated from core\configuration.cpp
  #define KEY(TYPE, VAR) VAR = Storage->Read ## TYPE(LASTELEM(UnicodeString(TEXT(#VAR))), VAR)
  #define KEYEX(TYPE, VAR, NAME) VAR = Storage->Read ## TYPE(NAME, VAR)
  #pragma warn -eas
  REGCONFIG(false);
  #pragma warn +eas
  #undef KEYEX
  #undef KEY

  DefaultHistory();
  if (Storage->OpenSubKey(L"History", false))
  {
    TStrings * Names = nullptr;
    try
    {
      Names = new TStringList();
      Storage->GetSubKeyNames(Names);
      for (int Index = 0; Index < Names->Count; Index++)
      {
        if (Storage->OpenSubKey(Names->Strings[Index], false))
        {
          THistoryStrings * HistoryStrings = nullptr;
          try
          {
            // remove defaults, if any
            int HIndex = FHistory->IndexOf(Names->Strings[Index]);
            if (HIndex >= 0)
            {
              THistoryStrings * DefaultStrings = dynamic_cast<THistoryStrings *>(FHistory->Objects[HIndex]);
              SAFE_DESTROY(DefaultStrings);
              FHistory->Delete(HIndex);
            }

            HistoryStrings = new THistoryStrings();
            Storage->ReadValues(HistoryStrings);
            FHistory->AddObject(Names->Strings[Index], HistoryStrings);
            HistoryStrings = nullptr;
          }
          __finally
          {
            Storage->CloseSubKey();
            SAFE_DESTROY(HistoryStrings);
          }
        }
      }
    }
    __finally
    {
      Storage->CloseSubKey();
      SAFE_DESTROY(Names);
    }
  }

  if (Storage->OpenSubKey(L"HistoryParams", false))
  {
    try
    {
      THistoryStrings * HistoryStrings;
      for (int Index = 0; Index < FHistory->Count; Index++)
      {
        HistoryStrings = dynamic_cast<THistoryStrings *>(FHistory->Objects[Index]);
        if (Storage->OpenSubKey(FHistory->Strings[Index], false))
        {
          try
          {
            for (int VIndex = 0; VIndex < HistoryStrings->Count; VIndex++)
            {
              void * Data;
              if (Storage->ReadBinaryData(IntToStr(VIndex), &Data, sizeof(Data)) ==
                      sizeof(Data))
              {
                HistoryStrings->Objects[VIndex] = reinterpret_cast<TObject *>(Data);
              }
            }
          }
          __finally
          {
            Storage->CloseSubKey();
          }
        }
      }
    }
    __finally
    {
      Storage->CloseSubKey();
    }
  }
}

void TCustomWinConfiguration::LoadAdmin(THierarchicalStorage * Storage)
{
  TGUIConfiguration::LoadAdmin(Storage);
  FDefaultInterface = TInterface(Storage->ReadInteger(L"DefaultInterfaceInterface", FDefaultInterface));
}

void TCustomWinConfiguration::RecryptPasswords(TStrings * RecryptPasswordErrors)
{
  TOperationVisualizer Visualizer;

  StoredSessions->RecryptPasswords(RecryptPasswordErrors);

  if (OnMasterPasswordRecrypt != nullptr)
  {
    try
    {
      OnMasterPasswordRecrypt(nullptr);
    }
    catch (Exception & E)
    {
      UnicodeString Message;
      if (ExceptionMessage(&E, Message))
      {
        // we do not expect this really to happen,
        // so we do not bother providing context
        RecryptPasswordErrors->Add(Message);
      }
    }
  }
}

void TCustomWinConfiguration::AskForMasterPasswordIfNotSetAndNeededToPersistSessionData(
  TSessionData * SessionData)
{
  if (!DisablePasswordStoring &&
      SessionData->HasAnyPassword() &&
      UseMasterPassword)
  {
    AskForMasterPasswordIfNotSet();
  }
}

void TCustomWinConfiguration::SetInterface(TInterface value)
{
  SET_CONFIG_PROPERTY(Interface);
}

void TCustomWinConfiguration::SetHistory(const UnicodeString Index,
  TStrings * value)
{
  int I = FHistory->IndexOf(Index);
  bool NonEmpty = (value != nullptr) && (value->Count > 0);
  THistoryStrings * HistoryStrings = nullptr;
  if (I >= 0)
  {
    HistoryStrings = dynamic_cast<THistoryStrings *>(FHistory->Objects[I]);
    if (HistoryStrings->Equals(value))
    {
      HistoryStrings = nullptr;
    }
  }
  else if (NonEmpty)
  {
    HistoryStrings = new THistoryStrings();
    FHistory->AddObject(Index, HistoryStrings);
  }

  if (HistoryStrings != nullptr)
  {
    if (NonEmpty)
    {
      HistoryStrings->Assign(value);
      while (HistoryStrings->Count > MaxHistoryCount)
      {
        HistoryStrings->Delete(HistoryStrings->Count - 1);
      }
    }
    else
    {
      HistoryStrings->Clear();
    }
    HistoryStrings->Modified = true;
  }
}

TStrings * TCustomWinConfiguration::GetHistory(const UnicodeString Index)
{
  int I = FHistory->IndexOf(Index);
  return I >= 0 ? dynamic_cast<TStrings *>(FHistory->Objects[I]) : FEmptyHistory;
}

UnicodeString TCustomWinConfiguration::GetValidHistoryKey(UnicodeString Key)
{
  for (int Index = 1; Index <= Key.Length(); Index++)
  {
    if (!IsLetter(Key[Index]) && !IsDigit(Key[Index]))
    {
      Key[Index] = L'_';
    }
  }

  while (!Key.IsEmpty() && (Key[1] == L'_'))
  {
    Key.Delete(1, 1);
  }

  while (!Key.IsEmpty() && (Key[Key.Length()] == L'_'))
  {
    Key.Delete(Key.Length(), 1);
  }

  int P;
  while ((P = Key.Pos(L"__")) > 0)
  {
    Key.Delete(P, 1);
  }

  return Key;
}

void TCustomWinConfiguration::SetSynchronizeChecklist(TSynchronizeChecklistConfiguration value)
{
  SET_CONFIG_PROPERTY(SynchronizeChecklist);
}

void TCustomWinConfiguration::SetFindFile(TFindFileConfiguration value)
{
  SET_CONFIG_PROPERTY(FindFile);
}

void TCustomWinConfiguration::SetConsoleWin(TConsoleWinConfiguration value)
{
  SET_CONFIG_PROPERTY(ConsoleWin);
}

void TCustomWinConfiguration::SetLoginDialog(TLoginDialogConfiguration value)
{
  SET_CONFIG_PROPERTY(LoginDialog);
}

void TCustomWinConfiguration::SetConfirmExitOnCompletion(bool value)
{
  SET_CONFIG_PROPERTY(ConfirmExitOnCompletion);
}

void TCustomWinConfiguration::SetSynchronizeSummary(bool value)
{
  SET_CONFIG_PROPERTY(SynchronizeSummary);
}

UnicodeString TCustomWinConfiguration::GetDefaultFixedWidthFontName()
{
  // These are defaults for respective version of Windows Notepad
  UnicodeString Result;
  if (!FDefaultFixedWidthFontName.IsEmpty())
  {
    Result = FDefaultFixedWidthFontName;
  }
  else if (IsWin8())
  {
    Result = L"Consolas";
  }
  else
  {
    Result = L"Lucida Console";
  }
  return Result;
}

int TCustomWinConfiguration::GetDefaultFixedWidthFontSize()
{
  // These are defaults for respective version of Windows Notepad
  int Result;
  if (FDefaultFixedWidthFontSize != 0)
  {
    Result = FDefaultFixedWidthFontSize;
  }
  else if (IsWin8())
  {
    Result = 11;
  }
  else
  {
    Result = 10;
  }
  return Result;
}
