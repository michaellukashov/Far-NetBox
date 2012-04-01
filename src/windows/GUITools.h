//---------------------------------------------------------------------------
#ifndef GUIToolsH
#define GUIToolsH
//---------------------------------------------------------------------------
#include "boostdefines.hpp"
#include <boost/signals/signal0.hpp>

// from shlobj.h
#define CSIDL_DESKTOP                   0x0000        // <desktop>
#define CSIDL_SENDTO                    0x0009        // <user name>\SendTo
#define CSIDL_DESKTOPDIRECTORY          0x0010        // <user name>\Desktop
#define CSIDL_COMMON_DESKTOPDIRECTORY   0x0019        // All Users\Desktop
#define CSIDL_APPDATA                   0x001a        // <user name>\Application Data
#define CSIDL_PROGRAM_FILES             0x0026        // C:\Program Files
#define CSIDL_PERSONAL                  0x0005        // My Documents
//---------------------------------------------------------------------------
#include <FileMasks.H>
//---------------------------------------------------------------------------
class TSessionData;
//---------------------------------------------------------------------------
typedef boost::signal0<void> processmessages_signal_type;
typedef processmessages_signal_type::slot_type processmessages_slot_type;
//---------------------------------------------------------------------------
bool FindFile(std::wstring &Path);
bool FileExistsEx(const std::wstring Path);
bool ExecuteShell(const std::wstring Path, const std::wstring Params);
bool ExecuteShell(const std::wstring Path, const std::wstring Params,
                  HANDLE &Handle);
bool ExecuteShellAndWait(HINSTANCE Handle, const std::wstring Path,
                         const std::wstring Params, const processmessages_signal_type &ProcessMessages);
bool ExecuteShellAndWait(HINSTANCE Handle, const std::wstring Command,
                         const processmessages_signal_type &ProcessMessages);
void OpenSessionInPutty(const std::wstring PuttyPath,
                        TSessionData *SessionData, const std::wstring Password);
bool SpecialFolderLocation(int PathID, std::wstring &Path);
std::wstring ItemsFormatString(const std::wstring SingleItemFormat,
                               const std::wstring MultiItemsFormat, size_t Count, const std::wstring FirstItem);
std::wstring ItemsFormatString(const std::wstring SingleItemFormat,
                               const std::wstring MultiItemsFormat, nb::TStrings *Items);
std::wstring FileNameFormatString(const std::wstring SingleFileFormat,
                                  const std::wstring MultiFileFormat, nb::TStrings *Files, bool Remote);
std::wstring FormatBytes(__int64 Bytes, bool UseOrders = true);
std::wstring UniqTempDir(const std::wstring BaseDir,
                         const std::wstring Identity, bool Mask = false);
bool DeleteDirectory(const std::wstring DirName);
std::wstring FormatDateTimeSpan(const std::wstring TimeFormat, nb::TDateTime DateTime);
//---------------------------------------------------------------------------
class TLocalCustomCommand : public TFileCustomCommand
{
public:
    TLocalCustomCommand();
    TLocalCustomCommand(const TCustomCommandData &Data, const std::wstring Path);
    TLocalCustomCommand(const TCustomCommandData &Data, const std::wstring Path,
                        const std::wstring FileName, const std::wstring LocalFileName,
                        const std::wstring FileList);

    virtual bool __fastcall IsFileCommand(const std::wstring Command);
    bool HasLocalFileName(const std::wstring Command);

protected:
    virtual size_t __fastcall PatternLen(size_t Index, char PatternCmd);
    virtual bool __fastcall PatternReplacement(size_t Index, const std::wstring Pattern,
                                    std::wstring &Replacement, bool &Delimit);
    virtual void __fastcall DelimitReplacement(const std::wstring Replacement, char Quote);

private:
    std::wstring FLocalFileName;
};
//---------------------------------------------------------------------------
#endif
