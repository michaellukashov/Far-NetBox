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

#ifndef _export
#define _export __declspec(dllexport)
#endif  //_export

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif  //_WIN32_WINNT

#ifndef _WIN32_IE
#define _WIN32_IE 0x0501
#endif  //_WIN32_IE

#pragma warning(push, 1)
#include <plugin.hpp>
#include <farkeys.hpp>
#include <farcolor.hpp>
#include <string>
#include <vector>
#include <list>
#include <map>
#include <set>
#include <iterator>
#include <algorithm>
#include <assert.h>
#pragma warning(pop)

using namespace std;

#define NETBOX_DEBUG

inline int __cdecl debug_printf(const wchar_t *format, ...)
{
    (void)format;
    int len = 0;
#ifdef NETBOX_DEBUG
    va_list args;
    va_start(args, format);
    len = _vscwprintf(format, args);
    wstring buf(len + sizeof(wchar_t), 0);
    vswprintf_s(&buf[0], buf.size(), format, args);

    va_end(args);
    OutputDebugStringW(buf.c_str());
#endif
    return len;
}

#ifdef NETBOX_DEBUG
#define DEBUG_PRINTF(format, ...) debug_printf(format, __VA_ARGS__);
#else
#define DEBUG_PRINTF(format, ...)
#endif

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

        wstring content(title);
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
    static wstring GetFormattedString(const int id, ...)
    {
        const wchar_t *errFmt = GetString(id);
        assert(errFmt);
        va_list args;
        va_start(args, id);
        const int len = _vscwprintf(errFmt, args) + 1 /* last NULL */;
        if (len == 1)
        {
            return wstring();
        }
        wstring ret(len, 0);
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
        static wstring pluginDir;
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
    static bool GetProcessingFileName(int openFrom, INT_PTR item, wstring &fileName)
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
            vector<unsigned char> ppiBuffer(ppiBufferLength);
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
typedef void __fastcall (__closure *TFarInputBoxValidateEvent)
(AnsiString &Text);
//---------------------------------------------------------------------------
enum
{
    FAR170BETA4 = MAKEFARVERSION(1, 70, 1282),
    FAR170BETA5 = MAKEFARVERSION(1, 70, 1634),
    FAR170ALPHA6 = MAKEFARVERSION(1, 70, 1812),
    FAR170 = MAKEFARVERSION(1, 70, 2087)
};
//---------------------------------------------------------------------------
const size_t StartupInfoMinSize = 372;
const size_t StandardFunctionsMinSize = 228;
//---------------------------------------------------------------------------
typedef void __fastcall (__closure *TFarMessageTimerEvent)(unsigned int &Result);
typedef void __fastcall (__closure *TFarMessageClickEvent)(void *Token, int Result, bool &Close);
//---------------------------------------------------------------------------
struct TFarMessageParams
{
    TFarMessageParams();

    TStrings *MoreMessages;
    AnsiString CheckBoxLabel;
    bool CheckBox;
    unsigned int Timer;
    unsigned int TimerAnswer;
    TFarMessageTimerEvent TimerEvent;
    unsigned int Timeout;
    unsigned int TimeoutButton;
    AnsiString TimeoutStr;
    TFarMessageClickEvent ClickEvent;
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
    __fastcall TCustomFarPlugin(HWND AHandle);
    virtual __fastcall ~TCustomFarPlugin();
    virtual int __fastcall GetMinFarVersion();
    virtual void __fastcall SetStartupInfo(const struct PluginStartupInfo *Info);
    virtual void __fastcall ExitFAR();
    virtual void __fastcall GetPluginInfo(struct PluginInfo *Info);
    virtual int __fastcall Configure(int Item);
    virtual void *__fastcall OpenPlugin(int OpenFrom, int Item);
    virtual void __fastcall ClosePlugin(void *Plugin);
    virtual void __fastcall GetOpenPluginInfo(HANDLE Plugin, struct OpenPluginInfo *Info);
    virtual int __fastcall GetFindData(HANDLE Plugin,
                                       struct PluginPanelItem **PanelItem, int *ItemsNumber, int OpMode);
    virtual void __fastcall FreeFindData(HANDLE Plugin, struct PluginPanelItem *PanelItem,
                                         int ItemsNumber);
    virtual int __fastcall ProcessHostFile(HANDLE Plugin,
                                           struct PluginPanelItem *PanelItem, int ItemsNumber, int OpMode);
    virtual int __fastcall ProcessKey(HANDLE Plugin, int Key, unsigned int ControlState);
    virtual int __fastcall ProcessEvent(HANDLE Plugin, int Event, void *Param);
    virtual int __fastcall SetDirectory(HANDLE Plugin, const char *Dir, int OpMode);
    virtual int __fastcall MakeDirectory(HANDLE Plugin, char *Name, int OpMode);
    virtual int __fastcall DeleteFiles(HANDLE Plugin, struct PluginPanelItem *PanelItem,
                                       int ItemsNumber, int OpMode);
    virtual int __fastcall GetFiles(HANDLE Plugin, struct PluginPanelItem *PanelItem,
                                    int ItemsNumber, int Move, char *DestPath, int OpMode);
    virtual int __fastcall PutFiles(HANDLE Plugin, struct PluginPanelItem *PanelItem,
                                    int ItemsNumber, int Move, int OpMode);
    virtual int __fastcall ProcessEditorEvent(int Event, void *Param);
    virtual int __fastcall ProcessEditorInput(const INPUT_RECORD *Rec);

    virtual void __fastcall HandleException(Exception *E, int OpMode = 0);

    static char *DuplicateStr(const AnsiString Str, bool AllowEmpty = false);
    int __fastcall Message(unsigned int Flags, const AnsiString Title,
                           const AnsiString Message, TStrings *Buttons = NULL,
                           TFarMessageParams *Params = NULL, bool Oem = false);
    int __fastcall MaxMessageLines();
    int __fastcall MaxMenuItemLength();
    int __fastcall Menu(unsigned int Flags, AnsiString Title,
                        AnsiString Bottom, TStrings *Items, const int *BreakKeys,
                        int &BreakCode);
    int __fastcall Menu(unsigned int Flags, const AnsiString Title,
                        const AnsiString Bottom, TStrings *Items);
    int __fastcall Menu(unsigned int Flags, const AnsiString Title,
                        const AnsiString Bottom, const FarMenuItem *Items, int Count,
                        const int *BreakKeys, int &BreakCode);
    bool __fastcall InputBox(AnsiString Title, AnsiString Prompt,
                             AnsiString &Text, unsigned long Flags, AnsiString HistoryName = "",
                             int MaxLen = 255, TFarInputBoxValidateEvent OnValidate = NULL);
    AnsiString __fastcall GetMsg(int MsgId);
    void __fastcall SaveScreen(THandle &Screen);
    void __fastcall RestoreScreen(THandle &Screen);
    bool __fastcall CheckForEsc();
    bool __fastcall Viewer(AnsiString FileName, unsigned int Flags,
                           AnsiString Title = "");
    bool __fastcall Editor(AnsiString FileName, unsigned int Flags,
                           AnsiString Title = "");

    int __fastcall FarAdvControl(int Command, void *Param = NULL);
    int __fastcall FarAdvControl(int Command, int Param);
    bool __fastcall FarControl(int Command, void *Param, HANDLE Plugin = INVALID_HANDLE_VALUE);
    int __fastcall FarEditorControl(int Command, void *Param);
    unsigned int __fastcall FarSystemSettings();
    void __fastcall Text(int X, int Y, int Color, AnsiString Str);
    void __fastcall FlushText();
    void __fastcall WriteConsole(AnsiString Str);
    void __fastcall FarCopyToClipboard(AnsiString Str);
    void __fastcall FarCopyToClipboard(TStrings *Strings);
    int __fastcall FarVersion();
    AnsiString __fastcall FormatFarVersion(int Version);
    AnsiString __fastcall TemporaryDir();
    int __fastcall InputRecordToKey(const INPUT_RECORD *Rec);
    TFarEditorInfo *__fastcall EditorInfo();

    void __fastcall ShowConsoleTitle(const AnsiString Title);
    void __fastcall ClearConsoleTitle();
    void __fastcall UpdateConsoleTitle(const AnsiString Title);
    void __fastcall UpdateConsoleTitleProgress(short Progress);
    void __fastcall ShowTerminalScreen();
    void __fastcall SaveTerminalScreen();
    void __fastcall ScrollTerminalScreen(int Rows);
    TPoint __fastcall TerminalInfo(TPoint *Size = NULL, TPoint *Cursor = NULL);
    unsigned int ConsoleWindowState();
    void __fastcall ToggleVideoMode();

    TCustomFarFileSystem *__fastcall GetPanelFileSystem(bool Another = false,
            HANDLE Plugin = INVALID_HANDLE_VALUE);

    __property AnsiString ModuleName = { read = GetModuleName };
    __property TFarDialog *TopDialog = { read = FTopDialog };
    __property HWND Handle = { read = FHandle };
    __property bool ANSIApis = { read = FANSIApis };
    __property unsigned int FarThread = { read = FFarThread };

protected:
    PluginStartupInfo FStartupInfo;
    FarStandardFunctions FFarStandardFunctions;
    HWND FHandle;
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

    virtual bool __fastcall HandlesFunction(THandlesFunction Function);
    virtual void __fastcall GetPluginInfoEx(long unsigned &Flags,
                                            TStrings *DiskMenuStrings, TStrings *PluginMenuStrings,
                                            TStrings *PluginConfigStrings, TStrings *CommandPrefixes) = 0;
    virtual TCustomFarFileSystem *__fastcall OpenPluginEx(int OpenFrom, int Item) = 0;
    virtual bool __fastcall ConfigureEx(int Item) = 0;
    virtual int __fastcall ProcessEditorEventEx(int Event, void *Param) = 0;
    virtual int __fastcall ProcessEditorInputEx(const INPUT_RECORD *Rec) = 0;
    virtual void __fastcall HandleFileSystemException(TCustomFarFileSystem *FileSystem,
            Exception *E, int OpMode = 0);
    virtual bool __fastcall IsOldFar();
    virtual void __fastcall OldFar();
    void __fastcall ResetCachedInfo();
    int __fastcall MaxLength(TStrings *Strings);
    int __fastcall FarMessage(unsigned int Flags,
                              const AnsiString Title, const AnsiString Message, TStrings *Buttons,
                              TFarMessageParams *Params);
    int __fastcall DialogMessage(unsigned int Flags,
                                 const AnsiString Title, const AnsiString Message, TStrings *Buttons,
                                 TFarMessageParams *Params);
    void __fastcall InvalidateOpenPluginInfo();

    __property TCriticalSection *CriticalSection = { read = FCriticalSection };

private:
    PluginInfo FPluginInfo;
    TStringList *FSavedTitles;
    AnsiString FCurrentTitle;
    short FCurrentProgress;

    void __fastcall ClearPluginInfo(PluginInfo &Info);
    AnsiString __fastcall GetModuleName();
    void __fastcall UpdateConsoleTitle();
    AnsiString __fastcall FormatConsoleTitle();
    HWND __fastcall GetConsoleWindow();
};
//---------------------------------------------------------------------------
class TCustomFarFileSystem : public TObject
{
    friend TFarPanelInfo;
    friend TCustomFarPlugin;
public:
    __fastcall TCustomFarFileSystem(TCustomFarPlugin *APlugin);
    virtual __fastcall ~TCustomFarFileSystem();
    void __fastcall GetOpenPluginInfo(struct OpenPluginInfo *Info);
    int __fastcall GetFindData(struct PluginPanelItem **PanelItem,
                               int *ItemsNumber, int OpMode);
    void __fastcall FreeFindData(struct PluginPanelItem *PanelItem, int ItemsNumber);
    int __fastcall ProcessHostFile(struct PluginPanelItem *PanelItem,
                                   int ItemsNumber, int OpMode);
    int __fastcall ProcessKey(int Key, unsigned int ControlState);
    int __fastcall ProcessEvent(int Event, void *Param);
    int __fastcall SetDirectory(const char *Dir, int OpMode);
    int __fastcall MakeDirectory(char *Name, int OpMode);
    int __fastcall DeleteFiles(struct PluginPanelItem *PanelItem,
                               int ItemsNumber, int OpMode);
    int __fastcall GetFiles(struct PluginPanelItem *PanelItem,
                            int ItemsNumber, int Move, char *DestPath, int OpMode);
    int __fastcall PutFiles(struct PluginPanelItem *PanelItem,
                            int ItemsNumber, int Move, int OpMode);
    virtual void __fastcall Close();

protected:
    TCustomFarPlugin *FPlugin;
    bool FClosed;

    virtual void __fastcall GetOpenPluginInfoEx(long unsigned &Flags,
            AnsiString &HostFile, AnsiString &CurDir, AnsiString &Format,
            AnsiString &PanelTitle, TFarPanelModes *PanelModes, int &StartPanelMode,
            int &StartSortMode, bool &StartSortOrder, TFarKeyBarTitles *KeyBarTitles,
            AnsiString &ShortcutData) = 0;
    virtual bool __fastcall GetFindDataEx(TList *PanelItems, int OpMode) = 0;
    virtual bool __fastcall ProcessHostFileEx(TList *PanelItems, int OpMode);
    virtual bool __fastcall ProcessKeyEx(int Key, unsigned int ControlState);
    virtual bool __fastcall ProcessEventEx(int Event, void *Param);
    virtual bool __fastcall SetDirectoryEx(const AnsiString Dir, int OpMode);
    virtual int __fastcall MakeDirectoryEx(AnsiString &Name, int OpMode);
    virtual bool __fastcall DeleteFilesEx(TList *PanelItems, int OpMode);
    virtual int __fastcall GetFilesEx(TList *PanelItems, bool Move,
                                      AnsiString &DestPath, int OpMode);
    virtual int __fastcall PutFilesEx(TList *PanelItems, bool Move, int OpMode);

    void __fastcall ResetCachedInfo();
    bool __fastcall FarControl(int Command, void *Param);
    bool __fastcall UpdatePanel(bool ClearSelection = false, bool Another = false);
    void __fastcall RedrawPanel(bool Another = false);
    void __fastcall ClosePlugin();
    AnsiString __fastcall GetMsg(int MsgId);
    TCustomFarFileSystem *__fastcall GetOppositeFileSystem();
    bool __fastcall IsActiveFileSystem();
    bool __fastcall IsLeft();
    bool __fastcall IsRight();

    virtual void __fastcall HandleException(Exception *E, int OpMode = 0);

    __property TFarPanelInfo *PanelInfo = { read = GetPanelInfo, index = 0 };
    __property TFarPanelInfo *AnotherPanelInfo = { read = GetPanelInfo, index = 1 };
    __property TCriticalSection *CriticalSection = { read = FCriticalSection };

protected:
    TCriticalSection *FCriticalSection;
    void __fastcall InvalidateOpenPluginInfo();

private:
    OpenPluginInfo FOpenPluginInfo;
    bool FOpenPluginInfoValid;
    TFarPanelInfo *FPanelInfo[2];
    static unsigned int FInstances;

    void __fastcall ClearOpenPluginInfo(OpenPluginInfo &Info);
    TList *__fastcall CreatePanelItemList(struct PluginPanelItem *PanelItem,
                                          int ItemsNumber);
    TFarPanelInfo *__fastcall GetPanelInfo(int Another);
};
//---------------------------------------------------------------------------
#define PANEL_MODES_COUNT 10
class TFarPanelModes : public TObject
{
    friend class TCustomFarFileSystem;
public:
    void __fastcall SetPanelMode(int Mode, const AnsiString ColumnTypes = "",
                                 const AnsiString ColumnWidths = "", TStrings *ColumnTitles = NULL,
                                 bool FullScreen = false, bool DetailedStatus = true, bool AlignExtensions = true,
                                 bool CaseConversion = true, const AnsiString StatusColumnTypes = "",
                                 const AnsiString StatusColumnWidths = "");

private:
    PanelMode FPanelModes[PANEL_MODES_COUNT];
    bool FReferenced;

    __fastcall TFarPanelModes();
    virtual __fastcall ~TFarPanelModes();

    void __fastcall FillOpenPluginInfo(struct OpenPluginInfo *Info);
    static void __fastcall ClearPanelMode(PanelMode &Mode);
    static int __fastcall CommaCount(const AnsiString ColumnTypes);
};
//---------------------------------------------------------------------------
class TFarKeyBarTitles : public TObject
{
    friend class TCustomFarFileSystem;
public:
    void __fastcall ClearFileKeyBarTitles();
    void __fastcall ClearKeyBarTitle(TFarShiftStatus ShiftStatus,
                                     int FunctionKeyStart, int FunctionKeyEnd = 0);
    void __fastcall SetKeyBarTitle(TFarShiftStatus ShiftStatus, int FunctionKey,
                                   const AnsiString Title);

private:
    KeyBarTitles FKeyBarTitles;
    bool FReferenced;

    __fastcall TFarKeyBarTitles();
    virtual __fastcall ~TFarKeyBarTitles();

    void __fastcall FillOpenPluginInfo(struct OpenPluginInfo *Info);
    static void __fastcall ClearKeyBarTitles(KeyBarTitles &Titles);
};
//---------------------------------------------------------------------------
class TCustomFarPanelItem : TObject
{
    friend class TCustomFarFileSystem;
public:

protected:
    virtual void __fastcall GetData(
        unsigned long &Flags, AnsiString &FileName, __int64 &Size,
        unsigned long &FileAttributes,
        TDateTime &LastWriteTime, TDateTime &LastAccess,
        unsigned long &NumberOfLinks, AnsiString &Description,
        AnsiString &Owner, void *& UserData, int &CustomColumnNumber) = 0;
    virtual AnsiString __fastcall CustomColumnData(int Column);

    void __fastcall FillPanelItem(struct PluginPanelItem *PanelItem);
};
//---------------------------------------------------------------------------
class TFarPanelItem : TCustomFarPanelItem
{
public:
    __fastcall TFarPanelItem(PluginPanelItem *APanelItem);
    __property unsigned long Flags = { read = GetFlags };
    __property unsigned long FileAttributes = { read = GetFileAttributes };
    __property AnsiString FileName = { read = GetFileName };
    __property void *UserData = { read = GetUserData };
    __property bool Selected = { read = GetSelected, write = SetSelected };
    __property bool IsParentDirectory = { read = GetIsParentDirectory };
    __property bool IsFile = { read = GetIsFile };

protected:
    PluginPanelItem *FPanelItem;

    virtual void __fastcall GetData(
        unsigned long &Flags, AnsiString &FileName, __int64 &Size,
        unsigned long &FileAttributes,
        TDateTime &LastWriteTime, TDateTime &LastAccess,
        unsigned long &NumberOfLinks, AnsiString &Description,
        AnsiString &Owner, void *& UserData, int &CustomColumnNumber);
    virtual AnsiString __fastcall CustomColumnData(int Column);

private:
    unsigned long __fastcall GetFlags();
    AnsiString __fastcall GetFileName();
    void *__fastcall GetUserData();
    bool __fastcall GetSelected();
    void __fastcall SetSelected(bool value);
    bool __fastcall GetIsParentDirectory();
    bool __fastcall GetIsFile();
    unsigned long __fastcall GetFileAttributes();
};
//---------------------------------------------------------------------------
class THintPanelItem : public TCustomFarPanelItem
{
public:
    __fastcall THintPanelItem(const AnsiString AHint);

protected:
    virtual void __fastcall GetData(
        unsigned long &Flags, AnsiString &FileName, __int64 &Size,
        unsigned long &FileAttributes,
        TDateTime &LastWriteTime, TDateTime &LastAccess,
        unsigned long &NumberOfLinks, AnsiString &Description,
        AnsiString &Owner, void *& UserData, int &CustomColumnNumber);

private:
    AnsiString FHint;
};
//---------------------------------------------------------------------------
enum TFarPanelType { ptFile, ptTree, ptQuickView, ptInfo };
//---------------------------------------------------------------------------
class TFarPanelInfo : public TObject
{
public:
    __fastcall TFarPanelInfo(PanelInfo *APanelInfo, TCustomFarFileSystem *AOwner);
    virtual __fastcall ~TFarPanelInfo();

    __property TList *Items = { read = GetItems };
    __property int ItemCount = { read = GetItemCount };
    __property int SelectedCount = { read = GetSelectedCount };
    __property TFarPanelItem *FocusedItem = { read = GetFocusedItem, write = SetFocusedItem };
    __property int FocusedIndex = { read = GetFocusedIndex, write = SetFocusedIndex };
    __property TRect Bounds = { read = GetBounds };
    __property TFarPanelType Type = { read = GetType };
    __property bool IsPlugin = { read = GetIsPlugin };
    __property AnsiString CurrentDirectory = { read = GetCurrentDirectory };

    void __fastcall ApplySelection();
    TFarPanelItem *__fastcall FindFileName(const AnsiString FileName);
    TFarPanelItem *__fastcall FindUserData(void *UserData);

private:
    PanelInfo *FPanelInfo;
    TList *FItems;
    TCustomFarFileSystem *FOwner;

    TList *__fastcall GetItems();
    TFarPanelItem *__fastcall GetFocusedItem();
    void __fastcall SetFocusedItem(TFarPanelItem *value);
    int __fastcall GetFocusedIndex();
    void __fastcall SetFocusedIndex(int value);
    int __fastcall GetItemCount();
    int __fastcall GetSelectedCount();
    TRect __fastcall GetBounds();
    TFarPanelType __fastcall GetType();
    bool __fastcall GetIsPlugin();
    AnsiString __fastcall GetCurrentDirectory();
};
//---------------------------------------------------------------------------
enum MENUITEMFLAGS_EX
{
    MIF_HIDDEN = 0x40000000UL,
};
//---------------------------------------------------------------------------
class TFarMenuItems : public TStringList
{
public:
    __fastcall TFarMenuItems();
    void __fastcall AddSeparator(bool Visible = true);
    HIDESBASE int __fastcall Add(AnsiString Text, bool Visible = true);

    virtual void __fastcall Clear();
    virtual void __fastcall Delete(int Index);

    __property int ItemFocused = { read = FItemFocused, write = SetItemFocused };

    __property bool Disabled[int Index] = { read = GetFlag, write = SetFlag, index = MIF_DISABLE };
    __property bool Checked[int Index] = { read = GetFlag, write = SetFlag, index = MIF_CHECKED };

protected:
    virtual void __fastcall PutObject(int Index, TObject *AObject);

private:
    int FItemFocused;

    void __fastcall SetItemFocused(int value);
    void __fastcall SetFlag(int Index, int Flag, bool Value);
    bool __fastcall GetFlag(int Index, int Flag);
};
//---------------------------------------------------------------------------
class TFarEditorInfo
{
public:
    __fastcall TFarEditorInfo(EditorInfo *Info);
    __fastcall ~TFarEditorInfo();

    __property int EditorID = { read = GetEditorID };
    __property AnsiString FileName = { read = GetFileName };

private:
    EditorInfo *FEditorInfo;

    int __fastcall GetEditorID();
    AnsiString __fastcall GetFileName();
};
//---------------------------------------------------------------------------
class TFarEnvGuard
{
public:
    inline __fastcall TFarEnvGuard();
    inline __fastcall ~TFarEnvGuard();
};
//---------------------------------------------------------------------------
class TFarPluginEnvGuard
{
public:
    __fastcall TFarPluginEnvGuard();
    __fastcall ~TFarPluginEnvGuard();

private:
    bool FANSIApis;
};
//---------------------------------------------------------------------------
void __fastcall FarWrapText(AnsiString Text, TStrings *Result, int MaxWidth);
//---------------------------------------------------------------------------
extern TCustomFarPlugin *FarPlugin;
//---------------------------------------------------------------------------
inline char *StrFromFar(char *S)
{
    OemToChar(S, S);
    return S;
}
//---------------------------------------------------------------------------
AnsiString StrFromFar(const char *S);
//---------------------------------------------------------------------------
inline char *StrFromFar(AnsiString &S)
{
    OemToChar(S.c_str(), S.c_str());
    return S.c_str();
}
//---------------------------------------------------------------------------
inline char *StrToFar(char *S)
{
    CharToOem(S, S);
    return S;
}
//---------------------------------------------------------------------------
inline char *StrToFar(AnsiString &S)
{
    S.Unique();
    CharToOem(S.c_str(), S.c_str());
    return S.c_str();
}
//---------------------------------------------------------------------------
#endif
