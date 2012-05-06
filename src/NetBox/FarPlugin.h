/**************************************************************************
 *  Far plugin helper (for FAR 2.0) (http://code.google.com/p/farplugs)   *
 *  Copyright (C) 2000-2011 by Artem Senichev <artemsen@gmail.com>        *
 *                                                                        *
 *  This program is free software: you can redistribute it and/or modify  *
 *  it under the terms of the GNU General Public License as published by  *
 *  the Free Software Foundation, either version 3 of the License, or     *
 *  (at your option) any later version.                                   *
 *                                                                        *
 *  This program is distributed in the hope that it will be useful,       *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *  GNU General Public License for more details.                          *
 *                                                                        *
 *  You should have received a copy of the GNU General Public License     *
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 **************************************************************************/

#pragma once

#include "boostdefines.hpp"
#include <boost/signals/signal3.hpp>

#pragma warning(push, 1)
#include "Classes.h"
#include "SysUtils.h"
#pragma warning(pop)
#include "Common.h"
#include "plugin.hpp"

//---------------------------------------------------------------------------
class TCustomFarFileSystem;
class TFarPanelModes;
class TFarKeyBarTitles;
class TFarPanelInfo;
class TFarDialog;
class TWinSCPFileSystem;
class TFarDialogItem;
class TCriticalSection;
class TFarMessageDialog;
class TFarEditorInfo;
class TFarPluginGuard;
//---------------------------------------------------------------------------
const int MaxMessageWidth = 64;
//---------------------------------------------------------------------------
enum TFarShiftStatus { fsNone, fsCtrl, fsAlt, fsShift, fsCtrlShift,
                       fsAltShift, fsCtrlAlt
                     };
enum THandlesFunction { hfProcessKey, hfProcessHostFile, hfProcessEvent };
typedef boost::signal1<void, UnicodeString &> farinputboxvalidate_signal_type;
typedef farinputboxvalidate_signal_type::slot_type farinputboxvalidate_slot_type;
//---------------------------------------------------------------------------
typedef boost::signal1<void, size_t &> farmessagetimer_signal_type;
typedef farmessagetimer_signal_type::slot_type farmessagetimer_slot_type;
typedef boost::signal3<void, void *, int, bool &> farmessageclick_signal_type;
typedef farmessageclick_signal_type::slot_type farmessageclick_slot_type;

//---------------------------------------------------------------------------
struct TFarMessageParams
{
    TFarMessageParams();

    TStrings *MoreMessages;
    UnicodeString CheckBoxLabel;
    bool CheckBox;
    size_t Timer;
    size_t TimerAnswer;
    farmessagetimer_slot_type *TimerEvent;
    size_t Timeout;
    size_t TimeoutButton;
    UnicodeString TimeoutStr;
    farmessageclick_slot_type *ClickEvent;
    void *Token;
};
//---------------------------------------------------------------------------
class TCustomFarPlugin : public TObject
{
    friend TCustomFarFileSystem;
    friend TFarDialog;
    friend TWinSCPFileSystem;
    friend TFarDialogItem;
    friend TFarMessageDialog;
    friend TFarPluginGuard;
public:
    explicit TCustomFarPlugin(HINSTANCE HInst);
    virtual ~TCustomFarPlugin();
    virtual int GetMinFarVersion();
    virtual void SetStartupInfo(const struct PluginStartupInfo *Info);
    virtual struct PluginStartupInfo *GetStartupInfo() { return &FStartupInfo; }
    virtual void ExitFAR();
    virtual void GetPluginInfo(struct PluginInfo *Info);
    virtual int Configure(int Item);
    virtual void *OpenPlugin(int OpenFrom, INT_PTR Item);
    virtual void ClosePlugin(void *Plugin);
    virtual void GetOpenPluginInfo(HANDLE Plugin, struct OpenPluginInfo *Info);
    virtual int GetFindData(HANDLE Plugin,
                            struct PluginPanelItem **PanelItem, int *ItemsNumber, int OpMode);
    virtual void FreeFindData(HANDLE Plugin, struct PluginPanelItem *PanelItem,
                              int ItemsNumber);
    virtual int ProcessHostFile(HANDLE Plugin,
                                struct PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);
    virtual int ProcessKey(HANDLE Plugin, int Key, unsigned int ControlState);
    virtual int ProcessEvent(HANDLE Plugin, int Event, void *Param);
    virtual int SetDirectory(HANDLE Plugin, const wchar_t *Dir, int OpMode);
    virtual int MakeDirectory(HANDLE Plugin, const wchar_t **Name, int OpMode);
    virtual int DeleteFiles(HANDLE Plugin, struct PluginPanelItem *PanelItem,
                            int ItemsNumber, int OpMode);
    virtual int GetFiles(HANDLE Plugin, struct PluginPanelItem *PanelItem,
                         int ItemsNumber, int Move, const wchar_t **DestPath, int OpMode);
    virtual int PutFiles(HANDLE Plugin, struct PluginPanelItem *PanelItem,
                         int ItemsNumber, int Move, int OpMode);
    virtual int ProcessEditorEvent(int Event, void *Param);
    virtual int ProcessEditorInput(const INPUT_RECORD *Rec);

    virtual void HandleException(const std::exception *E, int OpMode = 0);

    static wchar_t *DuplicateStr(const UnicodeString Str, bool AllowEmpty = false);
    int Message(unsigned int Flags, const UnicodeString Title,
                const UnicodeString Message, TStrings *Buttons = NULL,
                TFarMessageParams *Params = NULL);
    int MaxMessageLines();
    size_t MaxMenuItemLength();
    size_t Menu(unsigned int Flags, const UnicodeString Title,
             const UnicodeString Bottom, TStrings *Items, const int *BreakKeys,
             int &BreakCode);
    size_t Menu(unsigned int Flags, const UnicodeString Title,
             const UnicodeString Bottom, TStrings *Items);
    size_t Menu(unsigned int Flags, const UnicodeString Title,
             const UnicodeString Bottom, const FarMenuItem *Items, int Count,
             const int *BreakKeys, int &BreakCode);
    bool InputBox(const UnicodeString Title, const UnicodeString Prompt,
                  UnicodeString &Text, unsigned long Flags, const UnicodeString HistoryName = L"",
                  size_t MaxLen = 255, farinputboxvalidate_slot_type *OnValidate = NULL);
    UnicodeString GetMsg(int MsgId);
    void SaveScreen(HANDLE &Screen);
    void RestoreScreen(HANDLE &Screen);
    bool CheckForEsc();
    bool Viewer(const UnicodeString FileName, unsigned int Flags,
                UnicodeString Title = L"");
    bool Editor(const UnicodeString FileName, unsigned int Flags,
                UnicodeString Title = L"");

    INT_PTR FarAdvControl(int Command, void *Param = NULL);
    DWORD FarControl(int Command, int Param1, LONG_PTR Param2, HANDLE Plugin = INVALID_HANDLE_VALUE);
    int FarEditorControl(int Command, void *Param);
    INT_PTR FarSystemSettings();
    void Text(int X, int Y, int Color, const UnicodeString Str);
    void FlushText();
    void WriteConsole(const UnicodeString Str);
    void FarCopyToClipboard(const UnicodeString Str);
    void FarCopyToClipboard(TStrings *Strings);
    int FarVersion();
    UnicodeString FormatFarVersion(int Version);
    UnicodeString TemporaryDir();
    int InputRecordToKey(const INPUT_RECORD *Rec);
    TFarEditorInfo *EditorInfo();

    void ShowConsoleTitle(const UnicodeString Title);
    void ClearConsoleTitle();
    void UpdateConsoleTitle(const UnicodeString Title);
    void UpdateConsoleTitleProgress(short Progress);
    void ShowTerminalScreen();
    void SaveTerminalScreen();
    void ScrollTerminalScreen(int Rows);
    TPoint TerminalInfo(TPoint *Size = NULL, TPoint *Cursor = NULL);
    unsigned int ConsoleWindowState();
    void ToggleVideoMode();

    TCustomFarFileSystem *GetPanelFileSystem(bool Another = false,
            HANDLE Plugin = INVALID_HANDLE_VALUE);

    UnicodeString GetModuleName();
    TFarDialog *GetTopDialog() const { return FTopDialog; }
    HINSTANCE GetHandle() const { return FHandle; };
    bool GetANSIApis() const { return FANSIApis; };
    unsigned int GetFarThread() const { return FFarThread; };
    FarStandardFunctions &GetFarStandardFunctions() { return FFarStandardFunctions; }
protected:
    PluginStartupInfo FStartupInfo;
    FarStandardFunctions FFarStandardFunctions;
    HINSTANCE FHandle;
    bool FANSIApis;
    TObjectList *FOpenedPlugins;
    TFarDialog *FTopDialog;
    HANDLE FConsoleInput;
    HANDLE FConsoleOutput;
    int FFarVersion;
    bool FTerminalScreenShowing;
    TCriticalSection *FCriticalSection;
    unsigned int FFarThread;
    bool FValidFarSystemSettings;
    INT_PTR FFarSystemSettings;
    TPoint FNormalConsoleSize;
    TCustomFarPlugin *Self;

    virtual bool HandlesFunction(THandlesFunction Function);
    virtual void GetPluginInfoEx(long unsigned &Flags,
                                 TStrings *DiskMenuStrings, TStrings *PluginMenuStrings,
                                 TStrings *PluginConfigStrings, TStrings *CommandPrefixes) = 0;
    virtual TCustomFarFileSystem *OpenPluginEx(int OpenFrom, LONG_PTR Item) = 0;
    virtual bool ImportSessions() = 0;
    virtual bool ConfigureEx(int Item) = 0;
    virtual int ProcessEditorEventEx(int Event, void *Param) = 0;
    virtual int ProcessEditorInputEx(const INPUT_RECORD *Rec) = 0;
    virtual void HandleFileSystemException(TCustomFarFileSystem *FileSystem,
                                           const std::exception *E, int OpMode = 0);
    void ResetCachedInfo();
    size_t MaxLength(TStrings *Strings);
    int FarMessage(unsigned int Flags,
                   const UnicodeString Title, const UnicodeString Message, TStrings *Buttons,
                   TFarMessageParams *Params);
    int DialogMessage(unsigned int Flags,
                      const UnicodeString Title, const UnicodeString Message, TStrings *Buttons,
                      TFarMessageParams *Params);
    void InvalidateOpenPluginInfo();

    TCriticalSection *GetCriticalSection() const { return FCriticalSection; }

#ifdef NETBOX_DEBUG
public:
    void RunTests();
#endif
private:
    void UpdateProgress(int state, int progress);

private:
    PluginInfo FPluginInfo;
    TStringList *FSavedTitles;
    UnicodeString FCurrentTitle;
    short FCurrentProgress;

    void ClearPluginInfo(PluginInfo &Info);
    void UpdateConsoleTitle();
    UnicodeString FormatConsoleTitle();
    HWND GetConsoleWindow();
    RECT GetPanelBounds(HANDLE PanelHandle);
    bool CompareRects(const RECT &lhs, const RECT &rhs) const
    {
        return
            lhs.left == rhs.left &&
            lhs.top == rhs.top &&
            lhs.right == rhs.right &&
            lhs.bottom == rhs.bottom;
    }
};
//---------------------------------------------------------------------------
class TCustomFarFileSystem : public TObject
{
    friend TFarPanelInfo;
    friend TCustomFarPlugin;
public:
    TCustomFarFileSystem(TCustomFarPlugin *APlugin);
    virtual void Init();
    virtual ~TCustomFarFileSystem();

    void GetOpenPluginInfo(struct OpenPluginInfo *Info);
    int GetFindData(struct PluginPanelItem **PanelItem,
                    int *ItemsNumber, int OpMode);
    void FreeFindData(struct PluginPanelItem *PanelItem, int ItemsNumber);
    int ProcessHostFile(struct PluginPanelItem *PanelItem,
                        int ItemsNumber, int OpMode);
    int ProcessKey(int Key, unsigned int ControlState);
    int ProcessEvent(int Event, void *Param);
    int SetDirectory(const wchar_t *Dir, int OpMode);
    int MakeDirectory(const wchar_t **Name, int OpMode);
    int DeleteFiles(struct PluginPanelItem *PanelItem,
                    int ItemsNumber, int OpMode);
    int GetFiles(struct PluginPanelItem *PanelItem,
                 int ItemsNumber, int Move, const wchar_t **DestPath, int OpMode);
    int PutFiles(struct PluginPanelItem *PanelItem,
                 int ItemsNumber, int Move, int OpMode);
    virtual void Close();

protected:
    TCustomFarPlugin *FPlugin;
    bool FClosed;

    virtual void GetOpenPluginInfoEx(long unsigned &Flags,
                                     UnicodeString &HostFile, UnicodeString &CurDir, UnicodeString &Format,
                                     UnicodeString &PanelTitle, TFarPanelModes *PanelModes, int &StartPanelMode,
                                     int &StartSortMode, bool &StartSortOrder, TFarKeyBarTitles *KeyBarTitles,
                                     UnicodeString &ShortcutData) = 0;
    virtual bool GetFindDataEx(TObjectList *PanelItems, int OpMode) = 0;
    virtual bool ProcessHostFileEx(TObjectList *PanelItems, int OpMode);
    virtual bool ProcessKeyEx(int Key, unsigned int ControlState);
    virtual bool ProcessEventEx(int Event, void *Param);
    virtual bool SetDirectoryEx(const UnicodeString Dir, int OpMode);
    virtual int MakeDirectoryEx(UnicodeString &Name, int OpMode);
    virtual bool DeleteFilesEx(TObjectList *PanelItems, int OpMode);
    virtual int GetFilesEx(TObjectList *PanelItems, bool Move,
                           UnicodeString &DestPath, int OpMode);
    virtual int PutFilesEx(TObjectList *PanelItems, bool Move, int OpMode);

    void ResetCachedInfo();
    DWORD FarControl(int Command, int Param1, LONG_PTR Param2);
    DWORD FarControl(int Command, int Param1, LONG_PTR Param2, HANDLE Plugin);
    bool UpdatePanel(bool ClearSelection = false, bool Another = false);
    void RedrawPanel(bool Another = false);
    void ClosePlugin();
    UnicodeString GetMsg(int MsgId);
    TCustomFarFileSystem *GetOppositeFileSystem();
    bool IsActiveFileSystem();
    bool IsLeft();
    bool IsRight();

    virtual void HandleException(const std::exception *E, int OpMode = 0);

    TFarPanelInfo *GetPanelInfo() { return GetPanelInfo(0); };
    TFarPanelInfo *GetAnotherPanelInfo() { return GetPanelInfo(1); };
    TCriticalSection *GetCriticalSection() { return FCriticalSection; };

protected:
    TCriticalSection *FCriticalSection;
    void InvalidateOpenPluginInfo();

private:
    OpenPluginInfo FOpenPluginInfo;
    bool FOpenPluginInfoValid;
    TFarPanelInfo *FPanelInfo[2];
    static unsigned int FInstances;

    void ClearOpenPluginInfo(OpenPluginInfo &Info);
    TObjectList *CreatePanelItemList(struct PluginPanelItem *PanelItem,
                                         size_t ItemsNumber);
    TFarPanelInfo *GetPanelInfo(int Another);
};
//---------------------------------------------------------------------------
#define PANEL_MODES_COUNT 10
class TFarPanelModes : public TObject
{
    friend class TCustomFarFileSystem;
public:
    void SetPanelMode(size_t Mode, const UnicodeString ColumnTypes = L"",
                      const UnicodeString ColumnWidths = L"", TStrings *ColumnTitles = NULL,
                      bool FullScreen = false, bool DetailedStatus = true, bool AlignExtensions = true,
                      bool CaseConversion = true, const UnicodeString StatusColumnTypes = L"",
                      const UnicodeString StatusColumnWidths = L"");

private:
    PanelMode FPanelModes[PANEL_MODES_COUNT];
    bool FReferenced;

    TFarPanelModes();
    virtual ~TFarPanelModes();

    void FillOpenPluginInfo(struct OpenPluginInfo *Info);
    static void ClearPanelMode(PanelMode &Mode);
    static size_t CommaCount(const UnicodeString ColumnTypes);
};
//---------------------------------------------------------------------------
class TFarKeyBarTitles : public TObject
{
    friend class TCustomFarFileSystem;
public:
    void ClearFileKeyBarTitles();
    void ClearKeyBarTitle(TFarShiftStatus ShiftStatus,
                          int FunctionKeyStart, int FunctionKeyEnd = 0);
    void SetKeyBarTitle(TFarShiftStatus ShiftStatus, int FunctionKey,
                        const UnicodeString Title);

private:
    KeyBarTitles FKeyBarTitles;
    bool FReferenced;

    TFarKeyBarTitles();
    virtual ~TFarKeyBarTitles();

    void FillOpenPluginInfo(struct OpenPluginInfo *Info);
    static void ClearKeyBarTitles(KeyBarTitles &Titles);
};
//---------------------------------------------------------------------------
class TCustomFarPanelItem : public TObject
{
    friend class TCustomFarFileSystem;
public:

protected:
    virtual ~TCustomFarPanelItem()
    {}
    virtual void GetData(
        unsigned long &Flags, UnicodeString &FileName, __int64 &Size,
        unsigned long &FileAttributes,
        TDateTime &LastWriteTime, TDateTime &LastAccess,
        unsigned long &NumberOfLinks, UnicodeString &Description,
        UnicodeString &Owner, void *& UserData, int &CustomColumnNumber) = 0;
    virtual UnicodeString GetCustomColumnData(int Column);

    void FillPanelItem(struct PluginPanelItem *PanelItem);
};
//---------------------------------------------------------------------------
class TFarPanelItem : public TCustomFarPanelItem
{
public:
    explicit TFarPanelItem(PluginPanelItem *APanelItem);
    virtual ~TFarPanelItem();
    unsigned long GetFlags();
    unsigned long GetFileAttributes();
    UnicodeString GetFileName();
    void *GetUserData();
    bool GetSelected();
    void SetSelected(bool value);
    bool GetIsParentDirectory();
    bool GetIsFile();

protected:
    PluginPanelItem *FPanelItem;

    virtual void GetData(
        unsigned long &Flags, UnicodeString &FileName, __int64 &Size,
        unsigned long &FileAttributes,
        TDateTime &LastWriteTime, TDateTime &LastAccess,
        unsigned long &NumberOfLinks, UnicodeString &Description,
        UnicodeString &Owner, void *& UserData, int &CustomColumnNumber);
    virtual UnicodeString GetCustomColumnData(int Column);

private:
};
//---------------------------------------------------------------------------
class THintPanelItem : public TCustomFarPanelItem
{
public:
    explicit THintPanelItem(const UnicodeString AHint);
    virtual ~THintPanelItem()
    {}

protected:
    virtual void GetData(
        unsigned long &Flags, UnicodeString &FileName, __int64 &Size,
        unsigned long &FileAttributes,
        TDateTime &LastWriteTime, TDateTime &LastAccess,
        unsigned long &NumberOfLinks, UnicodeString &Description,
        UnicodeString &Owner, void *& UserData, int &CustomColumnNumber);

private:
    UnicodeString FHint;
};
//---------------------------------------------------------------------------
enum TFarPanelType { ptFile, ptTree, ptQuickView, ptInfo };
//---------------------------------------------------------------------------
class TFarPanelInfo : public TObject
{
public:
    explicit TFarPanelInfo(PanelInfo *APanelInfo, TCustomFarFileSystem *AOwner);
    virtual ~TFarPanelInfo();

    TObjectList *GetItems();
    size_t GetItemCount();
    size_t GetSelectedCount();
    TFarPanelItem *GetFocusedItem();
    void SetFocusedItem(TFarPanelItem *value);
    size_t GetFocusedIndex();
    void SetFocusedIndex(size_t value);
    TRect GetBounds();
    TFarPanelType GetType();
    bool GetIsPlugin();
    UnicodeString GetCurrentDirectory();

    void ApplySelection();
    TFarPanelItem *FindFileName(const UnicodeString FileName);
    TFarPanelItem *FindUserData(void *UserData);

private:
    PanelInfo *FPanelInfo;
    TObjectList *FItems;
    TCustomFarFileSystem *FOwner;
};
//---------------------------------------------------------------------------
enum MENUITEMFLAGS_EX
{
    // FIXME MIF_HIDDEN = 0x40000000UL,
};
//---------------------------------------------------------------------------
class TFarMenuItems : public TStringList
{
public:
    explicit TFarMenuItems();
    virtual ~TFarMenuItems()
    {}
    void __fastcall AddSeparator(bool Visible = true);
    virtual size_t __fastcall Add(const UnicodeString Text, bool Visible = true);

    virtual void __fastcall Clear();
    virtual void __fastcall Delete(size_t Index);

    size_t __fastcall GetItemFocused() { return FItemFocused; }
    void __fastcall SetItemFocused(size_t value);

    bool __fastcall GetDisabled(size_t Index) { return GetFlag(Index, MIF_DISABLE); }
    void __fastcall SetDisabled(size_t Index, bool value) { SetFlag(Index, MIF_DISABLE, value); }
    bool __fastcall GetChecked(size_t Index) { return GetFlag(Index, MIF_CHECKED); }
    void __fastcall SetChecked(size_t Index, bool value) { SetFlag(Index, MIF_CHECKED, value); }

protected:
    virtual void __fastcall PutObject(size_t Index, TObject *AObject);

private:
    size_t FItemFocused;

    void __fastcall SetFlag(size_t Index, size_t Flag, bool Value);
    bool __fastcall GetFlag(size_t Index, size_t Flag);
};
//---------------------------------------------------------------------------
class TFarEditorInfo
{
public:
    explicit TFarEditorInfo(EditorInfo *Info);
    ~TFarEditorInfo();

    int GetEditorID();
    UnicodeString GetFileName();

private:
    EditorInfo *FEditorInfo;

};
//---------------------------------------------------------------------------
class TFarEnvGuard
{
public:
    inline TFarEnvGuard();
    inline ~TFarEnvGuard();
};
//---------------------------------------------------------------------------
class TFarPluginEnvGuard
{
public:
    TFarPluginEnvGuard();
    ~TFarPluginEnvGuard();

private:
    bool FANSIApis;
};
//---------------------------------------------------------------------------
void FarWrapText(const UnicodeString Text, TStrings *Result, size_t MaxWidth);
//---------------------------------------------------------------------------
extern TCustomFarPlugin *FarPlugin;
