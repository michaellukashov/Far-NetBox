
#pragma once

#include <ctime>
#include <nbsystem.h>
#include <openssl/ssl.h>

class CFileZillaTools //: public TObject
{
CUSTOM_MEM_ALLOCATION_IMPL
public:
  virtual ~CFileZillaTools() = default;
  virtual void PreserveDownloadFileTime(HANDLE AHandle, void * UserData) = 0;
  virtual bool GetFileModificationTimeInUtc(const wchar_t * FileName, struct tm & Time) = 0;
  virtual wchar_t * LastSysErrorMessage() const = 0;
  virtual std::wstring GetClientString() const = 0;
  virtual void SetupSsl(ssl_st * Ssl) = 0;
  virtual std::wstring CustomReason(int Err) = 0;
};

