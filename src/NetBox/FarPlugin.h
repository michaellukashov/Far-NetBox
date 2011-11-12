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

class CFarPlugin
{
public:
    /**
     * Initialize plugin
     * \param psi plugin startup info pointer
     */
    static void Initialize(const PluginStartupInfo *psi)
    {
        assert(psi);

        PluginStartupInfo *psiStatic = AccessPSI(psi);
        static FarStandardFunctions fsfStatic = *psi->FSF;
        psiStatic->FSF = &fsfStatic;
    }

    /**
     * Far message box
     * \param title message title
     * \param text message text
     * \param flags message box flags
     * \return message box exit status
     */
    static int MessageBox(const wchar_t *title, const wchar_t *text, const int flags)
    {
        assert(title);
        assert(text);

        std::wstring content(title);
        content += L'\n';
        content += text;
        return GetPSI()->Message(GetPSI()->ModuleNumber, FMSG_ALLINONE | flags, NULL, reinterpret_cast<const wchar_t* const *>(content.c_str()), 0, 0);
    }

    /**
     * Get Far plugin string resource
     * \param id string resource Id
     * \return string
     */
    static const wchar_t *GetString(const int id)
    {
        return GetPSI()->GetMsg(GetPSI()->ModuleNumber, id);
    }

    /**
     * Get Far plugin formatted string
     * \param id string resource Id
     * \param ... additional params
     * \return formatted string
     */
    static std::wstring GetFormattedString(const int id, ...)
    {
        const wchar_t *errFmt = GetString(id);
        assert(errFmt);
        va_list args;
        va_start(args, id);
        const int len = _vscwprintf(errFmt, args) + 1 /* last NULL */;
        if (len == 1)
        {
            return std::wstring();
        }
        std::wstring ret(len, 0);
        vswprintf_s(&ret[0], ret.size(), errFmt, args);
        ret.erase(ret.length() - 1);        ///Trim last NULL
        return ret;
    }

    /**
     * Get plugin library path
     * \return plugin library path
     */
    static const wchar_t *GetPluginPath()
    {
        static std::wstring pluginDir;
        if (pluginDir.empty())
        {
            pluginDir = GetPSI()->ModuleName;
            pluginDir.resize(pluginDir.find_last_of(L'\\') + 1);
        }
        return pluginDir.c_str();
    }

    /**
     * Get processing file name
     * \param openFrom open from type (see Far help)
     * \param item item pointer (see Far help)
     * \param fileName processing file name
     * \return false if error
     */
    static bool GetProcessingFileName(int openFrom, INT_PTR item, std::wstring &fileName)
    {
        fileName.clear();

        //Determine file name
        if (openFrom == OPEN_COMMANDLINE)
        {
            wchar_t *cmdString = reinterpret_cast<wchar_t *>(item);
            GetPSI()->FSF->Unquote(cmdString);
            GetPSI()->FSF->Trim(cmdString);
            const int fileNameLen = GetPSI()->FSF->ConvertPath(CPM_FULL, cmdString, NULL, 0);
            if (fileNameLen)
            {
                fileName.resize(fileNameLen);
                GetPSI()->FSF->ConvertPath(CPM_FULL, cmdString, &fileName[0], fileNameLen);
                fileName.erase(fileName.length() - 1);  //last NULL
            }
        }
        else if (openFrom == OPEN_PLUGINSMENU)
        {
            PanelInfo pi;
            if (!GetPSI()->Control(PANEL_ACTIVE, FCTL_GETPANELINFO, sizeof(pi), reinterpret_cast<LONG_PTR>(&pi)))
            {
                return false;
            }

            const size_t ppiBufferLength = GetPSI()->Control(PANEL_ACTIVE, FCTL_GETPANELITEM, pi.CurrentItem, static_cast<LONG_PTR>(NULL));
            if (ppiBufferLength == 0)
            {
                return false;
            }
            std::vector<unsigned char> ppiBuffer(ppiBufferLength);
            PluginPanelItem *ppi = reinterpret_cast<PluginPanelItem *>(&ppiBuffer.front());
            if (!GetPSI()->Control(PANEL_ACTIVE, FCTL_GETPANELITEM, pi.CurrentItem, reinterpret_cast<LONG_PTR>(ppi)))
            {
                return false;
            }
            const int fileNameLen = GetPSI()->FSF->ConvertPath(CPM_FULL, ppi->FindData.lpwszFileName, NULL, 0);
            if (fileNameLen)
            {
                fileName.resize(fileNameLen);
                GetPSI()->FSF->ConvertPath(CPM_FULL, ppi->FindData.lpwszFileName, &fileName[0], fileNameLen);
                fileName.erase(fileName.length() - 1);  //last NULL
            }
        }
        else if (openFrom == OPEN_VIEWER)
        {
            ViewerInfo vi;
            ZeroMemory(&vi, sizeof(vi));
            vi.StructSize = sizeof(vi);
            GetPSI()->ViewerControl(VCTL_GETINFO, &vi);
            if (vi.FileName)
            {
                fileName = vi.FileName;
            }
        }
        else if (openFrom == OPEN_EDITOR)
        {
            const int buffLen = GetPSI()->EditorControl(ECTL_GETFILENAME, NULL);
            if (buffLen)
            {
                fileName.resize(buffLen + 1, 0);
                GetPSI()->EditorControl(ECTL_GETFILENAME, &fileName[0]);
            }
        }

        return !fileName.empty();
    }

public:
    /**
     * Call AdvControl function
     * \param command control command
     * \param param command parameter
     * \return call retcode
     */
    static INT_PTR AdvControl(const int command, void *param = NULL)
    {
        return GetPSI()->AdvControl(GetPSI()->ModuleNumber, command, param);
    }

    /**
     * Get plugin startup info pointer
     * \return plugin startup info pointer
     */
    static PluginStartupInfo *GetPSI()
    {
        assert(AccessPSI(NULL));
        return AccessPSI(NULL);
    }

private:
    /**
     * Access to internal static plugin startup info
     * \param psi new plugin startup info (NULL to save privious)
     * \return plugin startup info pointer
     */
    static PluginStartupInfo *AccessPSI(const PluginStartupInfo *psi)
    {
        static PluginStartupInfo psiStatic;
        if (psi)
        {
            psiStatic = *psi;
        }
        return &psiStatic;
    }
};

//---------------------------------------------------------------------------
class TCustomFarFileSystem;
class TFarPanelModes;
class TFarKeyBarTitles;
class TFarPanelInfo;
class TFarDialog;
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
// typedef void (*TFarInputBoxValidateEvent)(std::wstring &Text);
typedef boost::signal1<void, std::wstring &> farinputboxvalidate_signal_type;
typedef farinputboxvalidate_signal_type::slot_type farinputboxvalidate_slot_type;
//---------------------------------------------------------------------------
enum
{
    FAR170BETA4 = MAKEFARVERSION(1, 70, 1282),
    FAR170BETA5 = MAKEFARVERSION(1, 70, 1634),
    FAR170ALPHA6 = MAKEFARVERSION(1, 70, 1812),
    FAR170 = MAKEFARVERSION(1, 70, 2087),
    FAR20 = MAKEFARVERSION(2, 0, 1666),
};
//---------------------------------------------------------------------------
const size_t StartupInfoMinSize = 132; // 372;
const size_t StandardFunctionsMinSize = 228;
//---------------------------------------------------------------------------
// typedef void (*TFarMessageTimerEvent)(unsigned int &Result);
typedef boost::signal1<void, unsigned int &> farmessagetimer_signal_type;
typedef farmessagetimer_signal_type::slot_type farmessagetimer_slot_type;
// typedef void (*TFarMessageClickEvent)(void *Token, int Result, bool &Close);
typedef boost::signal3<void, void *, int, bool &> farmessageclick_signal_type;
typedef farmessageclick_signal_type::slot_type farmessageclick_slot_type;

//---------------------------------------------------------------------------
struct TFarMessageParams
{
    TFarMessageParams();

    TStrings *MoreMessages;
    std::wstring CheckBoxLabel;
    bool CheckBox;
    unsigned int Timer;
    unsigned int TimerAnswer;
    farmessagetimer_slot_type *TimerEvent;
    unsigned int Timeout;
    unsigned int TimeoutButton;
    std::wstring TimeoutStr;
    farmessageclick_slot_type *ClickEvent;
    void *Token;
};
//---------------------------------------------------------------------------
class TCustomFarPlugin : public TObject
{
    friend TCustomFarFileSystem;
    friend TFarDialog;
    friend TFarDialogItem;
    friend TFarMessageDialog;
    friend TFarPluginGuard;
public:
    explicit TCustomFarPlugin(HINSTANCE HInst);
    virtual ~TCustomFarPlugin();
    virtual int GetMinFarVersion();
    virtual void SetStartupInfo(const struct PluginStartupInfo *Info);
    virtual void ExitFAR();
    virtual void GetPluginInfo(struct PluginInfo *Info);
    virtual int Configure(int Item);
    virtual void *OpenPlugin(int OpenFrom, int Item);
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
    virtual int MakeDirectory(HANDLE Plugin, wchar_t *Name, int OpMode);
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
        const std::wstring Message, TStrings *Buttons = NULL,
        TFarMessageParams *Params = NULL, bool Oem = false);
    int MaxMessageLines();
    int MaxMenuItemLength();
    int Menu(unsigned int Flags, std::wstring Title,
        std::wstring Bottom, TStrings *Items, const int *BreakKeys,
        int &BreakCode);
    int Menu(unsigned int Flags, const std::wstring Title,
                        const std::wstring Bottom, TStrings *Items);
    int Menu(unsigned int Flags, const std::wstring Title,
        const std::wstring Bottom, const FarMenuItem *Items, int Count,
        const int *BreakKeys, int &BreakCode);
    bool InputBox(std::wstring Title, std::wstring Prompt,
        std::wstring &Text, unsigned long Flags, std::wstring HistoryName = L"",
        int MaxLen = 255, farinputboxvalidate_slot_type *OnValidate = NULL);
    std::wstring GetMsg(int MsgId);
    void SaveScreen(HANDLE &Screen);
    void RestoreScreen(HANDLE &Screen);
    bool CheckForEsc();
    bool Viewer(std::wstring FileName, unsigned int Flags,
        std::wstring Title = L"");
    bool Editor(std::wstring FileName, unsigned int Flags,
        std::wstring Title = L"");

    int FarAdvControl(int Command, void *Param = NULL);
    int FarAdvControl(int Command, int Param);
    DWORD FarControl(int Command, int Param1, LONG_PTR Param2, HANDLE Plugin = INVALID_HANDLE_VALUE);
    int FarEditorControl(int Command, void *Param);
    unsigned int FarSystemSettings();
    void Text(int X, int Y, int Color, std::wstring Str);
    void FlushText();
    void WriteConsole(std::wstring Str);
    void FarCopyToClipboard(std::wstring Str);
    void FarCopyToClipboard(TStrings *Strings);
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
    TPoint TerminalInfo(TPoint *Size = NULL, TPoint *Cursor = NULL);
    unsigned int ConsoleWindowState();
    void ToggleVideoMode();

    TCustomFarFileSystem *GetPanelFileSystem(bool Another = false,
        HANDLE Plugin = INVALID_HANDLE_VALUE);

    std::wstring GetModuleName();
    TFarDialog *GetTopDialog() const { return FTopDialog; }
    HINSTANCE GetHandle() const { return FHandle; };
    bool GetANSIApis() const { return FANSIApis; };
    unsigned int GetFarThread() const { return FFarThread; };
    FarStandardFunctions GetFarStandardFunctions() { return FFarStandardFunctions; }
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
    bool FOldFar;
    bool FValidFarSystemSettings;
    unsigned int FFarSystemSettings;
    TPoint FNormalConsoleSize;
    TCustomFarPlugin *Self;

    virtual bool HandlesFunction(THandlesFunction Function);
    virtual void GetPluginInfoEx(long unsigned &Flags,
        TStrings *DiskMenuStrings, TStrings *PluginMenuStrings,
        TStrings *PluginConfigStrings, TStrings *CommandPrefixes) = 0;
    virtual TCustomFarFileSystem *OpenPluginEx(int OpenFrom, int Item) = 0;
    virtual bool ConfigureEx(int Item) = 0;
    virtual int ProcessEditorEventEx(int Event, void *Param) = 0;
    virtual int ProcessEditorInputEx(const INPUT_RECORD *Rec) = 0;
    virtual void HandleFileSystemException(TCustomFarFileSystem *FileSystem,
        const std::exception *E, int OpMode = 0);
    virtual bool IsOldFar();
    virtual void OldFar();
    void ResetCachedInfo();
    int MaxLength(TStrings *Strings);
    int FarMessage(unsigned int Flags,
        const std::wstring Title, const std::wstring Message, TStrings *Buttons,
        TFarMessageParams *Params);
    int DialogMessage(unsigned int Flags,
        const std::wstring Title, const std::wstring Message, TStrings *Buttons,
        TFarMessageParams *Params);
    void InvalidateOpenPluginInfo();

    TCriticalSection *GetCriticalSection() const { return FCriticalSection; }

private:
    void RunTests();

private:
    PluginInfo FPluginInfo;
    TStringList *FSavedTitles;
    std::wstring FCurrentTitle;
    short FCurrentProgress;

    void ClearPluginInfo(PluginInfo &Info);
    void UpdateConsoleTitle();
    std::wstring FormatConsoleTitle();
    HWND GetConsoleWindow();
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
    int MakeDirectory(wchar_t *Name, int OpMode);
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
    virtual bool GetFindDataEx(TObjectList *PanelItems, int OpMode) = 0;
    virtual bool ProcessHostFileEx(TObjectList *PanelItems, int OpMode);
    virtual bool ProcessKeyEx(int Key, unsigned int ControlState);
    virtual bool ProcessEventEx(int Event, void *Param);
    virtual bool SetDirectoryEx(const std::wstring Dir, int OpMode);
    virtual int MakeDirectoryEx(std::wstring &Name, int OpMode);
    virtual bool DeleteFilesEx(TObjectList *PanelItems, int OpMode);
    virtual int GetFilesEx(TObjectList *PanelItems, bool Move,
        std::wstring &DestPath, int OpMode);
    virtual int PutFilesEx(TObjectList *PanelItems, bool Move, int OpMode);

    void ResetCachedInfo();
    DWORD FarControl(int Command, int Param1, LONG_PTR Param2);
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
    TObjectList *CreatePanelItemList(struct PluginPanelItem *PanelItem,
        int ItemsNumber);
    TFarPanelInfo *GetPanelInfo(int Another);
};
//---------------------------------------------------------------------------
#define PANEL_MODES_COUNT 10
class TFarPanelModes : public TObject
{
    friend class TCustomFarFileSystem;
public:
    void SetPanelMode(int Mode, const std::wstring ColumnTypes = L"",
        const std::wstring ColumnWidths = L"", TStrings *ColumnTitles = NULL,
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
    static int CommaCount(const std::wstring ColumnTypes);
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
class TCustomFarPanelItem : TObject
{
    friend class TCustomFarFileSystem;
public:

protected:
    virtual void GetData(
        unsigned long &Flags, std::wstring &FileName, __int64 &Size,
        unsigned long &FileAttributes,
        TDateTime &LastWriteTime, TDateTime &LastAccess,
        unsigned long &NumberOfLinks, std::wstring &Description,
        std::wstring &Owner, void *& UserData, int &CustomColumnNumber) = 0;
    virtual std::wstring GetCustomColumnData(int Column);

    void FillPanelItem(struct PluginPanelItem *PanelItem);
};
//---------------------------------------------------------------------------
class TFarPanelItem : TCustomFarPanelItem
{
public:
    TFarPanelItem(PluginPanelItem *APanelItem);
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
        TDateTime &LastWriteTime, TDateTime &LastAccess,
        unsigned long &NumberOfLinks, std::wstring &Description,
        std::wstring &Owner, void *& UserData, int &CustomColumnNumber);
    virtual std::wstring GetCustomColumnData(int Column);

private:
};
//---------------------------------------------------------------------------
class THintPanelItem : public TCustomFarPanelItem
{
public:
    THintPanelItem(const std::wstring AHint);

protected:
    virtual void GetData(
        unsigned long &Flags, std::wstring &FileName, __int64 &Size,
        unsigned long &FileAttributes,
        TDateTime &LastWriteTime, TDateTime &LastAccess,
        unsigned long &NumberOfLinks, std::wstring &Description,
        std::wstring &Owner, void *& UserData, int &CustomColumnNumber);

private:
    std::wstring FHint;
};
//---------------------------------------------------------------------------
enum TFarPanelType { ptFile, ptTree, ptQuickView, ptInfo };
//---------------------------------------------------------------------------
class TFarPanelInfo : public TObject
{
public:
    TFarPanelInfo(PanelInfo *APanelInfo, TCustomFarFileSystem *AOwner);
    virtual ~TFarPanelInfo();

    TObjectList *GetItems();
    int GetItemCount();
    int GetSelectedCount();
    TFarPanelItem *GetFocusedItem();
    void SetFocusedItem(TFarPanelItem *value);
    int GetFocusedIndex();
    void SetFocusedIndex(int value);
    TRect GetBounds();
    TFarPanelType GetType();
    bool GetIsPlugin();
    std::wstring GetCurrentDirectory();

    void ApplySelection();
    TFarPanelItem *FindFileName(const std::wstring FileName);
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
    TFarMenuItems();
    void AddSeparator(bool Visible = true);
    virtual int Add(std::wstring Text, bool Visible = true);

    virtual void Clear();
    virtual void Delete(int Index);

    int GetItemFocused() { return FItemFocused; }
    void SetItemFocused(int value);

    bool GetDisabled(int Index) { return GetFlag(Index, MIF_DISABLE); }
    void SetDisabled(int Index, bool value) { SetFlag(Index, MIF_DISABLE, value); }
    bool GetChecked(int Index) { return GetFlag(Index, MIF_CHECKED); }
    void SetChecked(int Index, bool value) { SetFlag(Index, MIF_CHECKED, value); }

protected:
    virtual void PutObject(int Index, TObject *AObject);

private:
    int FItemFocused;

    void SetFlag(int Index, int Flag, bool Value);
    bool GetFlag(int Index, int Flag);
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
void FarWrapText(std::wstring Text, TStrings *Result, int MaxWidth);
//---------------------------------------------------------------------------
extern TCustomFarPlugin *FarPlugin;
//---------------------------------------------------------------------------
inline std::wstring StrFromFar(wchar_t *S)
{
    // ::Error(SNotImplemented, 20);
    // OemToChar(S, S);
    return std::wstring(S);
}
//---------------------------------------------------------------------------
inline std::wstring StrFromFar(const wchar_t *S)
{
    // ::Error(SNotImplemented, 21);
    return std::wstring(S);
}
//---------------------------------------------------------------------------
inline std::wstring StrFromFar(std::wstring &S)
{
    // FIXME
    // ::Error(SNotImplemented, 22);
    // OemToChar(S.c_str(), S.c_str());
    return std::wstring(S);
    // return L"";
}
//---------------------------------------------------------------------------
/*
inline std::wstring StrToFar(wchar_t *S)
{
    // FIXME
    // ::Error(SNotImplemented, 23);
    // CharToOem(S, S);
    return std::wstring(S);
}
*/
//---------------------------------------------------------------------------

inline wchar_t *StrToFar(const std::wstring &S)
{
    // FIXME
    // ::Error(SNotImplemented, 24);
    // S.Unique();
    // CharToOem(S.c_str(), S.c_str());
    return (wchar_t *)S.c_str();
}

//---------------------------------------------------------------------------
inline wchar_t *StrToFar(const wchar_t *S)
{
    // FIXME
    // ::Error(SNotImplemented, 25);
    // S.Unique();
    // CharToOem(S, S);
    return (wchar_t *)S;
}
//---------------------------------------------------------------------------
