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
#pragma warning(pop)
#include "Common.h"

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
typedef boost::signal1<void, std::wstring &> farinputboxvalidate_signal_type;
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

    nb::TStrings *MoreMessages;
    std::wstring CheckBoxLabel;
    bool CheckBox;
    size_t Timer;
    size_t TimerAnswer;
    farmessagetimer_slot_type *TimerEvent;
    size_t Timeout;
    size_t TimeoutButton;
    std::wstring TimeoutStr;
    farmessageclick_slot_type *ClickEvent;
    void *Token;
};
//---------------------------------------------------------------------------
class TCustomFarPlugin : public nb::TObject
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

    static wchar_t *DuplicateStr(const std::wstring Str, bool AllowEmpty = false);
    int Message(unsigned int Flags, const std::wstring Title,
                const std::wstring Message, nb::TStrings *Buttons = NULL,
                TFarMessageParams *Params = NULL);
    int MaxMessageLines();
    size_t MaxMenuItemLength();
    size_t Menu(unsigned int Flags, const std::wstring Title,
             const std::wstring Bottom, nb::TStrings *Items, const int *BreakKeys,
             int &BreakCode);
    size_t Menu(unsigned int Flags, const std::wstring Title,
             const std::wstring Bottom, nb::TStrings *Items);
    size_t Menu(unsigned int Flags, const std::wstring Title,
             const std::wstring Bottom, const FarMenuItem *Items, int Count,
             const int *BreakKeys, int &BreakCode);
    bool InputBox(const std::wstring Title, const std::wstring Prompt,
                  std::wstring &Text, unsigned long Flags, const std::wstring HistoryName = L"",
                  size_t MaxLen = 255, farinputboxvalidate_slot_type *OnValidate = NULL);
    std::wstring GetMsg(int MsgId);
    void SaveScreen(HANDLE &Screen);
    void RestoreScreen(HANDLE &Screen);
    bool CheckForEsc();
    bool Viewer(const std::wstring FileName, unsigned int Flags,
                std::wstring Title = L"");
    bool Editor(const std::wstring FileName, unsigned int Flags,
                std::wstring Title = L"");

    INT_PTR FarAdvControl(int Command, void *Param = NULL);
    DWORD FarControl(int Command, int Param1, LONG_PTR Param2, HANDLE Plugin = INVALID_HANDLE_VALUE);
    int FarEditorControl(int Command, void *Param);
    INT_PTR FarSystemSettings();
    void Text(int X, int Y, int Color, const std::wstring Str);
    void FlushText();
    void WriteConsole(const std::wstring Str);
    void FarCopyToClipboard(const std::wstring Str);
    void FarCopyToClipboard(nb::TStrings *Strings);
    int FarVersion();
    std::wstring FormatFarVersion(int Version);
    std::wstring TemporaryDir();
    int InputRecordToKey(const INPUT_RECORD *Rec);
    TFarEditorInfo *EditorInfo();

    void ShowConsoleTitle(const std::wstring Title);
    void ClearConsoleTitle();
    void UpdateConsoleTitle(const std::wstring Title);
    void UpdateConsoleTitleProgress(short Progress);
    void ShowTerminalScreen();
    void SaveTerminalScreen();
    void ScrollTerminalScreen(int Rows);
    nb::TPoint TerminalInfo(nb::TPoint *Size = NULL, nb::TPoint *Cursor = NULL);
    unsigned int ConsoleWindowState();
    void ToggleVideoMode();

    TCustomFarFileSystem *GetPanelFileSystem(bool Another = false,
            HANDLE Plugin = INVALID_HANDLE_VALUE);

    std::wstring GetModuleName();
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
    nb::TObjectList *FOpenedPlugins;
    TFarDialog *FTopDialog;
    HANDLE FConsoleInput;
    HANDLE FConsoleOutput;
    int FFarVersion;
    bool FTerminalScreenShowing;
    TCriticalSection *FCriticalSection;
    unsigned int FFarThread;
    bool FValidFarSystemSettings;
    INT_PTR FFarSystemSettings;
    nb::TPoint FNormalConsoleSize;
    TCustomFarPlugin *Self;

    virtual bool HandlesFunction(THandlesFunction Function);
    virtual void GetPluginInfoEx(long unsigned &Flags,
                                 nb::TStrings *DiskMenuStrings, nb::TStrings *PluginMenuStrings,
                                 nb::TStrings *PluginConfigStrings, nb::TStrings *CommandPrefixes) = 0;
    virtual TCustomFarFileSystem *OpenPluginEx(int OpenFrom, LONG_PTR Item) = 0;
    virtual bool ImportSessions() = 0;
    virtual bool ConfigureEx(int Item) = 0;
    virtual int ProcessEditorEventEx(int Event, void *Param) = 0;
    virtual int ProcessEditorInputEx(const INPUT_RECORD *Rec) = 0;
    virtual void HandleFileSystemException(TCustomFarFileSystem *FileSystem,
                                           const std::exception *E, int OpMode = 0);
    void ResetCachedInfo();
    size_t MaxLength(nb::TStrings *Strings);
    int FarMessage(unsigned int Flags,
                   const std::wstring Title, const std::wstring Message, nb::TStrings *Buttons,
                   TFarMessageParams *Params);
    int DialogMessage(unsigned int Flags,
                      const std::wstring Title, const std::wstring Message, nb::TStrings *Buttons,
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
    nb::TStringList *FSavedTitles;
    std::wstring FCurrentTitle;
    short FCurrentProgress;

    void ClearPluginInfo(PluginInfo &Info);
    void UpdateConsoleTitle();
    std::wstring FormatConsoleTitle();
    HWND GetConsoleWindow();
};
//---------------------------------------------------------------------------
class TCustomFarFileSystem : public nb::TObject
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
                                     std::wstring &HostFile, std::wstring &CurDir, std::wstring &Format,
                                     std::wstring &PanelTitle, TFarPanelModes *PanelModes, int &StartPanelMode,
                                     int &StartSortMode, bool &StartSortOrder, TFarKeyBarTitles *KeyBarTitles,
                                     std::wstring &ShortcutData) = 0;
    virtual bool GetFindDataEx(nb::TObjectList *PanelItems, int OpMode) = 0;
    virtual bool ProcessHostFileEx(nb::TObjectList *PanelItems, int OpMode);
    virtual bool ProcessKeyEx(int Key, unsigned int ControlState);
    virtual bool ProcessEventEx(int Event, void *Param);
    virtual bool SetDirectoryEx(const std::wstring Dir, int OpMode);
    virtual int MakeDirectoryEx(std::wstring &Name, int OpMode);
    virtual bool DeleteFilesEx(nb::TObjectList *PanelItems, int OpMode);
    virtual int GetFilesEx(nb::TObjectList *PanelItems, bool Move,
                           std::wstring &DestPath, int OpMode);
    virtual int PutFilesEx(nb::TObjectList *PanelItems, bool Move, int OpMode);

    void ResetCachedInfo();
    DWORD FarControl(int Command, int Param1, LONG_PTR Param2);
    DWORD FarControl(int Command, int Param1, LONG_PTR Param2, HANDLE Plugin);
    bool UpdatePanel(bool ClearSelection = false, bool Another = false);
    void RedrawPanel(bool Another = false);
    void ClosePlugin();
    std::wstring GetMsg(int MsgId);
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
    nb::TObjectList *CreatePanelItemList(struct PluginPanelItem *PanelItem,
                                         size_t ItemsNumber);
    TFarPanelInfo *GetPanelInfo(int Another);
};
//---------------------------------------------------------------------------
#define PANEL_MODES_COUNT 10
class TFarPanelModes : public nb::TObject
{
    friend class TCustomFarFileSystem;
public:
    void SetPanelMode(size_t Mode, const std::wstring ColumnTypes = L"",
                      const std::wstring ColumnWidths = L"", nb::TStrings *ColumnTitles = NULL,
                      bool FullScreen = false, bool DetailedStatus = true, bool AlignExtensions = true,
                      bool CaseConversion = true, const std::wstring StatusColumnTypes = L"",
                      const std::wstring StatusColumnWidths = L"");

private:
    PanelMode FPanelModes[PANEL_MODES_COUNT];
    bool FReferenced;

    TFarPanelModes();
    virtual ~TFarPanelModes();

    void FillOpenPluginInfo(struct OpenPluginInfo *Info);
    static void ClearPanelMode(PanelMode &Mode);
    static size_t CommaCount(const std::wstring ColumnTypes);
};
//---------------------------------------------------------------------------
class TFarKeyBarTitles : public nb::TObject
{
    friend class TCustomFarFileSystem;
public:
    void ClearFileKeyBarTitles();
    void ClearKeyBarTitle(TFarShiftStatus ShiftStatus,
                          int FunctionKeyStart, int FunctionKeyEnd = 0);
    void SetKeyBarTitle(TFarShiftStatus ShiftStatus, int FunctionKey,
                        const std::wstring Title);

private:
    KeyBarTitles FKeyBarTitles;
    bool FReferenced;

    TFarKeyBarTitles();
    virtual ~TFarKeyBarTitles();

    void FillOpenPluginInfo(struct OpenPluginInfo *Info);
    static void ClearKeyBarTitles(KeyBarTitles &Titles);
};
//---------------------------------------------------------------------------
class TCustomFarPanelItem : public nb::TObject
{
    friend class TCustomFarFileSystem;
public:

protected:
    virtual ~TCustomFarPanelItem()
    {}
    virtual void GetData(
        unsigned long &Flags, std::wstring &FileName, __int64 &Size,
        unsigned long &FileAttributes,
        nb::TDateTime &LastWriteTime, nb::TDateTime &LastAccess,
        unsigned long &NumberOfLinks, std::wstring &Description,
        std::wstring &Owner, void *& UserData, int &CustomColumnNumber) = 0;
    virtual std::wstring GetCustomColumnData(int Column);

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
    std::wstring GetFileName();
    void *GetUserData();
    bool GetSelected();
    void SetSelected(bool value);
    bool GetIsParentDirectory();
    bool GetIsFile();

protected:
    PluginPanelItem *FPanelItem;

    virtual void GetData(
        unsigned long &Flags, std::wstring &FileName, __int64 &Size,
        unsigned long &FileAttributes,
        nb::TDateTime &LastWriteTime, nb::TDateTime &LastAccess,
        unsigned long &NumberOfLinks, std::wstring &Description,
        std::wstring &Owner, void *& UserData, int &CustomColumnNumber);
    virtual std::wstring GetCustomColumnData(int Column);

private:
};
//---------------------------------------------------------------------------
class THintPanelItem : public TCustomFarPanelItem
{
public:
    explicit THintPanelItem(const std::wstring AHint);
    virtual ~THintPanelItem()
    {}

protected:
    virtual void GetData(
        unsigned long &Flags, std::wstring &FileName, __int64 &Size,
        unsigned long &FileAttributes,
        nb::TDateTime &LastWriteTime, nb::TDateTime &LastAccess,
        unsigned long &NumberOfLinks, std::wstring &Description,
        std::wstring &Owner, void *& UserData, int &CustomColumnNumber);

private:
    std::wstring FHint;
};
//---------------------------------------------------------------------------
enum TFarPanelType { ptFile, ptTree, ptQuickView, ptInfo };
//---------------------------------------------------------------------------
class TFarPanelInfo : public nb::TObject
{
public:
    explicit TFarPanelInfo(PanelInfo *APanelInfo, TCustomFarFileSystem *AOwner);
    virtual ~TFarPanelInfo();

    nb::TObjectList *GetItems();
    size_t GetItemCount();
    size_t GetSelectedCount();
    TFarPanelItem *GetFocusedItem();
    void SetFocusedItem(TFarPanelItem *value);
    size_t GetFocusedIndex();
    void SetFocusedIndex(size_t value);
    nb::TRect GetBounds();
    TFarPanelType GetType();
    bool GetIsPlugin();
    std::wstring GetCurrentDirectory();

    void ApplySelection();
    TFarPanelItem *FindFileName(const std::wstring FileName);
    TFarPanelItem *FindUserData(void *UserData);

private:
    PanelInfo *FPanelInfo;
    nb::TObjectList *FItems;
    TCustomFarFileSystem *FOwner;
};
//---------------------------------------------------------------------------
enum MENUITEMFLAGS_EX
{
    // FIXME MIF_HIDDEN = 0x40000000UL,
};
//---------------------------------------------------------------------------
class TFarMenuItems : public nb::TStringList
{
public:
    explicit TFarMenuItems();
    virtual ~TFarMenuItems()
    {}
    void AddSeparator(bool Visible = true);
    virtual size_t Add(const std::wstring Text, bool Visible = true);

    virtual void Clear();
    virtual void Delete(size_t Index);

    size_t GetItemFocused() { return FItemFocused; }
    void SetItemFocused(size_t value);

    bool GetDisabled(size_t Index) { return GetFlag(Index, MIF_DISABLE); }
    void SetDisabled(size_t Index, bool value) { SetFlag(Index, MIF_DISABLE, value); }
    bool GetChecked(size_t Index) { return GetFlag(Index, MIF_CHECKED); }
    void SetChecked(size_t Index, bool value) { SetFlag(Index, MIF_CHECKED, value); }

protected:
    virtual void PutObject(size_t Index, nb::TObject *AObject);

private:
    size_t FItemFocused;

    void SetFlag(size_t Index, size_t Flag, bool Value);
    bool GetFlag(size_t Index, size_t Flag);
};
//---------------------------------------------------------------------------
class TFarEditorInfo
{
public:
    explicit TFarEditorInfo(EditorInfo *Info);
    ~TFarEditorInfo();

    int GetEditorID();
    std::wstring GetFileName();

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
void FarWrapText(const std::wstring Text, nb::TStrings *Result, size_t MaxWidth);
//---------------------------------------------------------------------------
extern TCustomFarPlugin *FarPlugin;
