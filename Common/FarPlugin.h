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

//---------------------------------------------------------------------------
class TObject;
typedef void (TObject::*TThreadMethod)();
typedef void (TObject::*TNotifyEvent)(TObject *);
//---------------------------------------------------------------------------
class TObject
{
public:
    TObject() :
        FOwnsObjects(false)
    {}
    virtual ~TObject()
    {}

    virtual void Change()
    {}
    void OwnsObjects(bool value) { FOwnsObjects = value; }

private:
    bool FOwnsObjects;
};

struct TPoint
{
    int x;
    int y;
    TPoint() :
        x(0),
        y(0)
    {}
    TPoint(int x, int y) :
        x(x),
        y(y)
    {}
};

struct TRect
{
    int Left;
    int Top;
    int Right;
    int Bottom;
    int Width() const { return Right - Left; }
    int Height() const { return Bottom - Top; }
    TRect() :
        Left(0),
        Top(0),
        Right(0),
        Bottom(0)
    {}
    TRect(int left, int top, int right, int bottom) :
        Left(left),
        Top(top),
        Right(right),
        Bottom(bottom)
    {}
    bool operator == (const TRect &other)
    {
        return
            Left == other.Left &&
            Top == other.Top &&
            Right == other.Right &&
            Bottom == other.Bottom;
    }
    bool operator != (const TRect &other)
    {
        return !(operator == (other));
    }
    bool operator == (const RECT &other)
    {
        return
            Left == other.left &&
            Top == other.top &&
            Right == other.right &&
            Bottom == other.bottom;
    }
    bool operator != (const RECT &other)
    {
        return !(operator == (other));
    }
};

class TPersistent : public TObject
{
public:
    virtual void Assign(TPersistent *Source)
    {}
};

class TObjectList : public TPersistent
{
public:
    size_t GetCount() const { return m_objects.size(); }

    TObject * operator [](size_t Index) const
    {
        return m_objects[Index];
    }
    TObject * GetItem(size_t Index) const
    {
        return m_objects[Index];
    }

    size_t Add(TObject *value)
    {
        m_objects.push_back(value);
        return m_objects.size() - 1;
    }
    void Remove(TObject *value)
    {
    }
    size_t IndexOf(TObject *value) const
    {
        return -1;
    }
    bool GetOwnsObjects() { return FOwnsObjects; }
    void SetOwnsObjects(bool value) { FOwnsObjects = value; }

private:
    vector<TObject *> m_objects;
    bool FOwnsObjects;
};

class TList : public TObjectList
{
public:
};

class TStrings : public TPersistent
{
public:
    virtual size_t GetCount()
    {
        return 0;
    }
    wstring GetString(int Index)
    {
        return L"";
    }
    wstring GetText()
    {
        return L"";
    }
    void *GetObject(int Index)
    {
        return NULL;
    }

    TNotifyEvent GetOnChange() { return FOnChange; }
    void SetOnChange(TNotifyEvent Event) { FOnChange = Event; }
    bool Equals(TStrings *value)
    {
        return false;
    }
private:
    TNotifyEvent FOnChange;
};

class TStringList : public TStrings
{
public:
    virtual void Assign(TPersistent *Source)
    {}
    void Put(int Index, wstring value)
    {
    }
    int GetUpdateCount()
    {
        return 0;
    }
    void Changed()
    {
    }
    size_t IndexOf(const wchar_t *value)
    {
        return -1;
    }
    size_t Add(wstring value)
    {
    }
    void Delete(size_t Index)
    {
    }
    void SetString(int Index, wstring S)
    {
    }
};

class TDateTime
{
public:
    operator double()
    {
        return 0.0;
    }
};

TDateTime Now()
{
    TDateTime result;
    return result;
}

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
typedef void (*TFarInputBoxValidateEvent)(wstring &Text);
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
typedef void (*TFarMessageTimerEvent)(unsigned int &Result);
typedef void (*TFarMessageClickEvent)(void *Token, int Result, bool &Close);
//---------------------------------------------------------------------------
struct TFarMessageParams
{
    TFarMessageParams();

    TStrings *MoreMessages;
    wstring CheckBoxLabel;
    bool CheckBox;
    unsigned int Timer;
    unsigned int TimerAnswer;
    TFarMessageTimerEvent TimerEvent;
    unsigned int Timeout;
    unsigned int TimeoutButton;
    wstring TimeoutStr;
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
    TCustomFarPlugin(HWND AHandle);
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
    virtual int SetDirectory(HANDLE Plugin, const char *Dir, int OpMode);
    virtual int MakeDirectory(HANDLE Plugin, char *Name, int OpMode);
    virtual int DeleteFiles(HANDLE Plugin, struct PluginPanelItem *PanelItem,
                                       int ItemsNumber, int OpMode);
    virtual int GetFiles(HANDLE Plugin, struct PluginPanelItem *PanelItem,
                                    int ItemsNumber, int Move, char *DestPath, int OpMode);
    virtual int PutFiles(HANDLE Plugin, struct PluginPanelItem *PanelItem,
                                    int ItemsNumber, int Move, int OpMode);
    virtual int ProcessEditorEvent(int Event, void *Param);
    virtual int ProcessEditorInput(const INPUT_RECORD *Rec);

    virtual void HandleException(exception *E, int OpMode = 0);

    static wchar_t *DuplicateStr(const wstring Str, bool AllowEmpty = false);
    int Message(unsigned int Flags, const wstring Title,
                           const wstring Message, TStrings *Buttons = NULL,
                           TFarMessageParams *Params = NULL, bool Oem = false);
    int MaxMessageLines();
    int MaxMenuItemLength();
    int Menu(unsigned int Flags, wstring Title,
                        wstring Bottom, TStrings *Items, const int *BreakKeys,
                        int &BreakCode);
    int Menu(unsigned int Flags, const wstring Title,
                        const wstring Bottom, TStrings *Items);
    int Menu(unsigned int Flags, const wstring Title,
                        const wstring Bottom, const FarMenuItem *Items, int Count,
                        const int *BreakKeys, int &BreakCode);
    bool InputBox(wstring Title, wstring Prompt,
                             wstring &Text, unsigned long Flags, wstring HistoryName = L"",
                             int MaxLen = 255, TFarInputBoxValidateEvent OnValidate = NULL);
    wstring GetMsg(int MsgId);
    void SaveScreen(HANDLE &Screen);
    void RestoreScreen(HANDLE &Screen);
    bool CheckForEsc();
    bool Viewer(wstring FileName, unsigned int Flags,
                           wstring Title = L"");
    bool Editor(wstring FileName, unsigned int Flags,
                           wstring Title = L"");

    int FarAdvControl(int Command, void *Param = NULL);
    int FarAdvControl(int Command, int Param);
    bool FarControl(int Command, int Param1, LONG_PTR Param2, HANDLE Plugin = INVALID_HANDLE_VALUE);
    int FarEditorControl(int Command, void *Param);
    unsigned int FarSystemSettings();
    void Text(int X, int Y, int Color, wstring Str);
    void FlushText();
    void WriteConsole(wstring Str);
    void FarCopyToClipboard(wstring Str);
    void FarCopyToClipboard(TStrings *Strings);
    int FarVersion();
    wstring FormatFarVersion(int Version);
    wstring TemporaryDir();
    int InputRecordToKey(const INPUT_RECORD *Rec);
    TFarEditorInfo *EditorInfo();

    void ShowConsoleTitle(const wstring Title);
    void ClearConsoleTitle();
    void UpdateConsoleTitle(const wstring Title);
    void UpdateConsoleTitleProgress(short Progress);
    void ShowTerminalScreen();
    void SaveTerminalScreen();
    void ScrollTerminalScreen(int Rows);
    TPoint TerminalInfo(TPoint *Size = NULL, TPoint *Cursor = NULL);
    unsigned int ConsoleWindowState();
    void ToggleVideoMode();

    TCustomFarFileSystem *GetPanelFileSystem(bool Another = false,
            HANDLE Plugin = INVALID_HANDLE_VALUE);

    wstring GetModuleName();
    TFarDialog *GetTopDialog() const { return FTopDialog; }
    HWND GetHandle() const { return FHandle; };
    bool GetANSIApis() const { return FANSIApis; };
    unsigned int GetFarThread() const { return FFarThread; };
    FarStandardFunctions GetFarStandardFunctions() { return FFarStandardFunctions; }
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

    virtual bool HandlesFunction(THandlesFunction Function);
    virtual void GetPluginInfoEx(long unsigned &Flags,
                                            TStrings *DiskMenuStrings, TStrings *PluginMenuStrings,
                                            TStrings *PluginConfigStrings, TStrings *CommandPrefixes) = 0;
    virtual TCustomFarFileSystem *OpenPluginEx(int OpenFrom, int Item) = 0;
    virtual bool ConfigureEx(int Item) = 0;
    virtual int ProcessEditorEventEx(int Event, void *Param) = 0;
    virtual int ProcessEditorInputEx(const INPUT_RECORD *Rec) = 0;
    virtual void HandleFileSystemException(TCustomFarFileSystem *FileSystem,
            exception *E, int OpMode = 0);
    virtual bool IsOldFar();
    virtual void OldFar();
    void ResetCachedInfo();
    int MaxLength(TStrings *Strings);
    int FarMessage(unsigned int Flags,
                              const wstring Title, const wstring Message, TStrings *Buttons,
                              TFarMessageParams *Params);
    int DialogMessage(unsigned int Flags,
                                 const wstring Title, const wstring Message, TStrings *Buttons,
                                 TFarMessageParams *Params);
    void InvalidateOpenPluginInfo();

    TCriticalSection *GetCriticalSection() const { return FCriticalSection; }

private:
    PluginInfo FPluginInfo;
    TStringList *FSavedTitles;
    wstring FCurrentTitle;
    short FCurrentProgress;

    void ClearPluginInfo(PluginInfo &Info);
    void UpdateConsoleTitle();
    wstring FormatConsoleTitle();
    HWND GetConsoleWindow();
};
//---------------------------------------------------------------------------
class TCustomFarFileSystem : public TObject
{
    friend TFarPanelInfo;
    friend TCustomFarPlugin;
public:
    TCustomFarFileSystem(TCustomFarPlugin *APlugin);
    virtual ~TCustomFarFileSystem();
    void GetOpenPluginInfo(struct OpenPluginInfo *Info);
    int GetFindData(struct PluginPanelItem **PanelItem,
                               int *ItemsNumber, int OpMode);
    void FreeFindData(struct PluginPanelItem *PanelItem, int ItemsNumber);
    int ProcessHostFile(struct PluginPanelItem *PanelItem,
                                   int ItemsNumber, int OpMode);
    int ProcessKey(int Key, unsigned int ControlState);
    int ProcessEvent(int Event, void *Param);
    int SetDirectory(const char *Dir, int OpMode);
    int MakeDirectory(char *Name, int OpMode);
    int DeleteFiles(struct PluginPanelItem *PanelItem,
                               int ItemsNumber, int OpMode);
    int GetFiles(struct PluginPanelItem *PanelItem,
                            int ItemsNumber, int Move, char *DestPath, int OpMode);
    int PutFiles(struct PluginPanelItem *PanelItem,
                            int ItemsNumber, int Move, int OpMode);
    virtual void Close();

protected:
    TCustomFarPlugin *FPlugin;
    bool FClosed;

    virtual void GetOpenPluginInfoEx(long unsigned &Flags,
            wstring &HostFile, wstring &CurDir, wstring &Format,
            wstring &PanelTitle, TFarPanelModes *PanelModes, int &StartPanelMode,
            int &StartSortMode, bool &StartSortOrder, TFarKeyBarTitles *KeyBarTitles,
            wstring &ShortcutData) = 0;
    virtual bool GetFindDataEx(TList *PanelItems, int OpMode) = 0;
    virtual bool ProcessHostFileEx(TList *PanelItems, int OpMode);
    virtual bool ProcessKeyEx(int Key, unsigned int ControlState);
    virtual bool ProcessEventEx(int Event, void *Param);
    virtual bool SetDirectoryEx(const wstring Dir, int OpMode);
    virtual int MakeDirectoryEx(wstring &Name, int OpMode);
    virtual bool DeleteFilesEx(TList *PanelItems, int OpMode);
    virtual int GetFilesEx(TList *PanelItems, bool Move,
                                      wstring &DestPath, int OpMode);
    virtual int PutFilesEx(TList *PanelItems, bool Move, int OpMode);

    void ResetCachedInfo();
    bool FarControl(int Command, void *Param);
    bool UpdatePanel(bool ClearSelection = false, bool Another = false);
    void RedrawPanel(bool Another = false);
    void ClosePlugin();
    wstring GetMsg(int MsgId);
    TCustomFarFileSystem *GetOppositeFileSystem();
    bool IsActiveFileSystem();
    bool IsLeft();
    bool IsRight();

    virtual void HandleException(exception *E, int OpMode = 0);

    TFarPanelInfo *GetPanelInfo() const { return GetPanelInfo(0); };
    TFarPanelInfo *GetAnotherPanelInfo() const { return GetPanelInfo(1); };
    TCriticalSection *GetCriticalSection() const { return FCriticalSection; };

protected:
    TCriticalSection *FCriticalSection;
    void InvalidateOpenPluginInfo();

private:
    OpenPluginInfo FOpenPluginInfo;
    bool FOpenPluginInfoValid;
    TFarPanelInfo *FPanelInfo[2];
    static unsigned int FInstances;

    void ClearOpenPluginInfo(OpenPluginInfo &Info);
    TList *CreatePanelItemList(struct PluginPanelItem *PanelItem,
                                          int ItemsNumber);
    TFarPanelInfo *GetPanelInfo(int Another) const;
};
//---------------------------------------------------------------------------
#define PANEL_MODES_COUNT 10
class TFarPanelModes : public TObject
{
    friend class TCustomFarFileSystem;
public:
    void SetPanelMode(int Mode, const wstring ColumnTypes = L"",
        const wstring ColumnWidths = L"", TStrings *ColumnTitles = NULL,
        bool FullScreen = false, bool DetailedStatus = true, bool AlignExtensions = true,
        bool CaseConversion = true, const wstring StatusColumnTypes = L"",
        const wstring StatusColumnWidths = L"");

private:
    PanelMode FPanelModes[PANEL_MODES_COUNT];
    bool FReferenced;

    TFarPanelModes();
    virtual ~TFarPanelModes();

    void FillOpenPluginInfo(struct OpenPluginInfo *Info);
    static void ClearPanelMode(PanelMode &Mode);
    static int CommaCount(const wstring ColumnTypes);
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
                                   const wstring Title);

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
        unsigned long &Flags, wstring &FileName, __int64 &Size,
        unsigned long &FileAttributes,
        TDateTime &LastWriteTime, TDateTime &LastAccess,
        unsigned long &NumberOfLinks, wstring &Description,
        wstring &Owner, void *& UserData, int &CustomColumnNumber) = 0;
    virtual wstring CustomColumnData(int Column);

    void FillPanelItem(struct PluginPanelItem *PanelItem);
};
//---------------------------------------------------------------------------
class TFarPanelItem : TCustomFarPanelItem
{
public:
    TFarPanelItem(PluginPanelItem *APanelItem);
    unsigned long GetFlags();
    unsigned long GetFileAttributes() const;
    wstring GetFileName();
    void *GetUserData();
    bool GetSelected();
    void SetSelected(bool value);
    bool GetIsParentDirectory();
    bool GetIsFile();

protected:
    PluginPanelItem *FPanelItem;

    virtual void GetData(
        unsigned long &Flags, wstring &FileName, __int64 &Size,
        unsigned long &FileAttributes,
        TDateTime &LastWriteTime, TDateTime &LastAccess,
        unsigned long &NumberOfLinks, wstring &Description,
        wstring &Owner, void *& UserData, int &CustomColumnNumber);
    virtual wstring CustomColumnData(int Column);

private:
};
//---------------------------------------------------------------------------
class THintPanelItem : public TCustomFarPanelItem
{
public:
    THintPanelItem(const wstring AHint);

protected:
    virtual void GetData(
        unsigned long &Flags, wstring &FileName, __int64 &Size,
        unsigned long &FileAttributes,
        TDateTime &LastWriteTime, TDateTime &LastAccess,
        unsigned long &NumberOfLinks, wstring &Description,
        wstring &Owner, void *& UserData, int &CustomColumnNumber);

private:
    wstring FHint;
};
//---------------------------------------------------------------------------
enum TFarPanelType { ptFile, ptTree, ptQuickView, ptInfo };
//---------------------------------------------------------------------------
class TFarPanelInfo : public TObject
{
public:
    TFarPanelInfo(PanelInfo *APanelInfo, TCustomFarFileSystem *AOwner);
    virtual ~TFarPanelInfo();

    TList *GetItems();
    int GetItemCount();
    int GetSelectedCount();
    TFarPanelItem *GetFocusedItem();
    void SetFocusedItem(TFarPanelItem *value);
    int GetFocusedIndex();
    void SetFocusedIndex(int value);
    TRect GetBounds();
    TFarPanelType GetType();
    bool GetIsPlugin();
    wstring GetCurrentDirectory();

    void ApplySelection();
    TFarPanelItem *FindFileName(const wstring FileName);
    TFarPanelItem *FindUserData(void *UserData);

private:
    PanelInfo *FPanelInfo;
    TList *FItems;
    TCustomFarFileSystem *FOwner;
};
//---------------------------------------------------------------------------
enum MENUITEMFLAGS_EX
{
    // MIF_HIDDEN = 0x40000000UL,
};
//---------------------------------------------------------------------------
class TFarMenuItems : public TStringList
{
public:
    TFarMenuItems();
    void AddSeparator(bool Visible = true);
    virtual int Add(wstring Text, bool Visible = true);

    virtual void Clear();
    virtual void Delete(int Index);

    int GetItemFocused() { return FItemFocused; }
    void SetItemFocused(int value);

    bool GetDisabled() { return GetFlag(MIF_DISABLE); }
    void SetDisabled(bool value) { SetFlag(MIF_DISABLE, value); }
    bool GetChecked() { return GetFlag(MIF_CHECKED); }
    void SetChecked(bool value) { SetFlag(MIF_CHECKED, value); }

protected:
    virtual void PutObject(int Index, TObject *AObject);

private:
    int FItemFocused;

    void SetFlag(int Flag, bool Value);
    bool GetFlag(int Flag);
};
//---------------------------------------------------------------------------
class TFarEditorInfo
{
public:
    TFarEditorInfo(EditorInfo *Info);
    ~TFarEditorInfo();

    int GetEditorID();
    wstring GetFileName();

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
void FarWrapText(wstring Text, TStrings *Result, int MaxWidth);
//---------------------------------------------------------------------------
extern TCustomFarPlugin *FarPlugin;
//---------------------------------------------------------------------------
inline wchar_t *StrFromFar(wchar_t *S)
{
    // OemToChar(S, S);
    return S;
}
//---------------------------------------------------------------------------
wstring StrFromFar(const wchar_t *S);
//---------------------------------------------------------------------------
inline char *StrFromFar(wstring &S)
{
    // OemToChar(S.c_str(), S.c_str());
    // return S.c_str();
    return "";
}
//---------------------------------------------------------------------------
inline wchar_t *StrToFar(wchar_t *S)
{
    // CharToOem(S, S);
    return S;
}
//---------------------------------------------------------------------------
inline wchar_t *StrToFar(wstring &S)
{
    // S.Unique();
    // CharToOem(S.c_str(), S.c_str());
    // return S.c_str();
    return L"";
}
//---------------------------------------------------------------------------

// from winscp434source\core\Common.h
#define FLAGSET(SET, FLAG) (((SET) & (FLAG)) == (FLAG))
#define LENOF(x) ( (sizeof((x))) / (sizeof(*(x))))
#define FLAGCLEAR(SET, FLAG) (((SET) & (FLAG)) == 0)
#define FLAGMASK(ENABLE, FLAG) ((ENABLE) ? (FLAG) : 0)
#define SAFE_DESTROY_EX(CLASS, OBJ) { CLASS * PObj = OBJ; OBJ = NULL; delete PObj; }
#define SAFE_DESTROY(OBJ) SAFE_DESTROY_EX(TObject, OBJ)
