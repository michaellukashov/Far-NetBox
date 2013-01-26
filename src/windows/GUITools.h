//---------------------------------------------------------------------------
#ifndef GUIToolsH
#define GUIToolsH

#include <CoreDefs.hpp>
//---------------------------------------------------------------------------
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
DEFINE_CALLBACK_TYPE0(TProcessMessagesEvent, void);
//---------------------------------------------------------------------------
bool FindFile(UnicodeString & Path);
bool FindTool(const UnicodeString & Name, UnicodeString & Path);
bool FileExistsEx(UnicodeString Path);
bool ExecuteShell(const UnicodeString & Path, const UnicodeString & Params);
bool ExecuteShell(const UnicodeString & Path, const UnicodeString & Params,
  HANDLE & Handle);
bool ExecuteShellAndWait(HINSTANCE Handle, const UnicodeString & Path,
  const UnicodeString & Params, TProcessMessagesEvent ProcessMessages);
bool ExecuteShellAndWait(HINSTANCE Handle, const UnicodeString & Command,
  TProcessMessagesEvent ProcessMessages);
void OpenSessionInPutty(const UnicodeString & PuttyPath,
  TSessionData * SessionData, UnicodeString Password);
bool SpecialFolderLocation(int PathID, UnicodeString & Path);
UnicodeString ItemsFormatString(const UnicodeString & SingleItemFormat,
  const UnicodeString & MultiItemsFormat, intptr_t Count, const UnicodeString & FirstItem);
UnicodeString ItemsFormatString(const UnicodeString & SingleItemFormat,
  const UnicodeString & MultiItemsFormat, TStrings * Items);
UnicodeString FileNameFormatString(const UnicodeString & SingleFileFormat,
  const UnicodeString & MultiFileFormat, TStrings * Files, bool Remote);
UnicodeString UniqTempDir(const UnicodeString & BaseDir,
  const UnicodeString & Identity, bool Mask = false);
bool DeleteDirectory(const UnicodeString & DirName);
UnicodeString FormatDateTimeSpan(const UnicodeString & TimeFormat, TDateTime DateTime);
//---------------------------------------------------------------------------
class TLocalCustomCommand : public TFileCustomCommand
{
public:
  TLocalCustomCommand();
  explicit TLocalCustomCommand(const TCustomCommandData & Data, const UnicodeString & Path);
  explicit TLocalCustomCommand(const TCustomCommandData & Data, const UnicodeString & Path,
    const UnicodeString & FileName, const UnicodeString & LocalFileName,
    const UnicodeString & FileList);
  virtual ~TLocalCustomCommand() {}

  virtual bool IsFileCommand(const UnicodeString & Command);
  bool HasLocalFileName(const UnicodeString & Command);

protected:
  virtual intptr_t PatternLen(intptr_t Index, wchar_t PatternCmd);
  virtual bool PatternReplacement(intptr_t Index, const UnicodeString & Pattern,
    UnicodeString & Replacement, bool & Delimit);
  virtual void DelimitReplacement(UnicodeString & Replacement, wchar_t Quote);

private:
  UnicodeString FLocalFileName;
};
//---------------------------------------------------------------------------
extern const UnicodeString PageantTool;
extern const UnicodeString PuttygenTool;
//---------------------------------------------------------------------------
#endif
