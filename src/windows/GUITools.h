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
bool FindFile(UnicodeString &Path);
bool FileExistsEx(const UnicodeString Path);
bool ExecuteShell(const UnicodeString Path, const UnicodeString Params);
bool ExecuteShell(const UnicodeString Path, const UnicodeString Params,
                  HANDLE &Handle);
bool ExecuteShellAndWait(HINSTANCE Handle, const UnicodeString Path,
                         const UnicodeString Params, const processmessages_signal_type &ProcessMessages);
bool ExecuteShellAndWait(HINSTANCE Handle, const UnicodeString Command,
                         const processmessages_signal_type &ProcessMessages);
void OpenSessionInPutty(const UnicodeString PuttyPath,
                        TSessionData *SessionData, const UnicodeString Password);
bool SpecialFolderLocation(int PathID, UnicodeString &Path);
UnicodeString ItemsFormatString(const UnicodeString SingleItemFormat,
                               const UnicodeString MultiItemsFormat, size_t Count, const UnicodeString FirstItem);
UnicodeString ItemsFormatString(const UnicodeString SingleItemFormat,
                               const UnicodeString MultiItemsFormat, TStrings *Items);
UnicodeString FileNameFormatString(const UnicodeString SingleFileFormat,
                                  const UnicodeString MultiFileFormat, TStrings *Files, bool Remote);
UnicodeString FormatBytes(__int64 Bytes, bool UseOrders = true);
UnicodeString UniqTempDir(const UnicodeString BaseDir,
                         const UnicodeString Identity, bool Mask = false);
bool DeleteDirectory(const UnicodeString DirName);
UnicodeString FormatDateTimeSpan(const UnicodeString TimeFormat, TDateTime DateTime);
//---------------------------------------------------------------------------
class TLocalCustomCommand : public TFileCustomCommand
{
public:
    TLocalCustomCommand();
    TLocalCustomCommand(const TCustomCommandData &Data, const UnicodeString Path);
    TLocalCustomCommand(const TCustomCommandData &Data, const UnicodeString Path,
                        const UnicodeString FileName, const UnicodeString LocalFileName,
                        const UnicodeString FileList);

    virtual bool __fastcall IsFileCommand(const UnicodeString Command);
    bool HasLocalFileName(const UnicodeString Command);

protected:
    virtual size_t __fastcall PatternLen(size_t Index, char PatternCmd);
    virtual bool __fastcall PatternReplacement(size_t Index, const UnicodeString Pattern,
                                    UnicodeString &Replacement, bool &Delimit);
    virtual void __fastcall DelimitReplacement(const UnicodeString Replacement, char Quote);

private:
    UnicodeString FLocalFileName;
};
//---------------------------------------------------------------------------
#endif
