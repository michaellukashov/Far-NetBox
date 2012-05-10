#include "nbafx.h"

#include "Common.h"
#include "FarUtil.h"
#include "FarTexts.h"

UnicodeString GetSystemErrorMessage(const DWORD errCode)
{
  assert(errCode);

  UnicodeString errorMsg;

  wchar_t codeNum[16];
  swprintf_s(codeNum, L"[0x%08X]", errCode);
  errorMsg = codeNum;

  wchar_t errInfoBuff[256];
  ZeroMemory(errInfoBuff, sizeof(errInfoBuff));
  if (!FormatMessageW(FORMAT_MESSAGE_FROM_SYSTEM, NULL, errCode, 0, errInfoBuff, sizeof(errInfoBuff) / sizeof(wchar_t), NULL))
    if (!FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE, GetModuleHandle(L"Wininet.dll"), errCode, 0, errInfoBuff, sizeof(errInfoBuff) / sizeof(wchar_t), NULL))
    {
      FormatMessageW(FORMAT_MESSAGE_FROM_HMODULE, GetModuleHandle(L"Winhttp.dll"), errCode, 0, errInfoBuff, sizeof(errInfoBuff) / sizeof(wchar_t), NULL);
    }
  //Remove '\r\n' from the end
  wchar_t * c = *errInfoBuff ? &errInfoBuff[wcslen(errInfoBuff) - 1] : NULL;
  while (*errInfoBuff && (*c == L'\n') || (*c == L'\r'))
  {
    *c = 0;
    c = *errInfoBuff ? &errInfoBuff[wcslen(errInfoBuff) - 1] : NULL;
  }

  if (*errInfoBuff)
  {
    errorMsg += L": ";
    errorMsg += errInfoBuff;
  }

  return errorMsg;
}


void ParseURL(const wchar_t * url, UnicodeString * scheme, UnicodeString * hostName, unsigned short * port, UnicodeString * path, UnicodeString * query, UnicodeString * userName, UnicodeString * password)
{
  assert(url);

  UnicodeString urlParse(url);

  //Parse scheme name
  int delimScheme = urlParse.Pos(L"://");
  if (delimScheme > 0)
  {
    if (scheme)
    {
      *scheme = urlParse.SubString(1, delimScheme - 1);
      // transform(scheme->begin(), scheme->end(), scheme->begin(), tolower);
      scheme->Lower();
    }
    urlParse.Delete(1, delimScheme + 2);
  }

  //Parse path
  int delimPath = urlParse.Pos(L'/');
  if (delimPath > 0)
  {
    UnicodeString parsePath = urlParse.SubString(delimPath);
    urlParse.Delete(delimPath, -1);
    //Parse query
    int delimQuery = 0;
    if (parsePath.RPos(delimQuery, L'?'))
    {
      if (query)
      {
        *query = parsePath.SubString(delimQuery);
      }
      parsePath.Delete(1, delimQuery);
    }
    if (path)
    {
      *path = parsePath;
    }
  }
  if (path && path->IsEmpty())
  {
    *path = L'/';
  }

  //Parse user name/password
  int delimLogin = 0;
  if (urlParse.RPos(delimLogin, L'@'))
  {
    UnicodeString parseLogin = urlParse.SubString(1, delimLogin);
    int delimPwd = 0;
    if (parseLogin.RPos(delimPwd, L':'))
    {
      if (password)
      {
        *password = parseLogin.SubString(delimPwd + 1);
      }
      parseLogin.Delete(delimPwd, -1);
    }
    if (userName)
    {
      *userName = parseLogin;
    }
    urlParse.Delete(1, delimLogin + 1);
  }

  //Parse port
  if (port)
  {
    *port = 0;
  }

  int delimPort = 0;
  if (urlParse.RPos(delimPort, L':'))
  {
    if (port)
    {
      const UnicodeString portNum = urlParse.SubString(delimPort + 1);
      *port = static_cast<unsigned short>(_wtoi(portNum.c_str()));
    }
    urlParse.Delete(delimPort, -1);
  }

  if (hostName)
  {
    *hostName = urlParse;
  }
}

FILETIME UnixTimeToFileTime(const time_t t)
{
  FILETIME ft;
  const LONGLONG ll = Int32x32To64(t, 10000000) + 116444736000000000;
  ft.dwLowDateTime = static_cast<DWORD>(ll);
  ft.dwHighDateTime = ll >> 32;
  return ft;
}

unsigned long TextToNumber(const UnicodeString text)
{
  return static_cast<unsigned long>(_wtoi(text.c_str()));
}

std::string NumberToText(int number)
{
  char codeText[16];
  _itoa_s(number, codeText, 10);
  return std::string(codeText);
}

UnicodeString NumberToWString(unsigned long number)
{
  wchar_t toText[16];
  _itow_s(number, toText, 10);
  return UnicodeString(toText);
}

void CheckAbortEvent(HANDLE * AbortEvent)
{
  assert(AbortEvent);
  static HANDLE stdIn = GetStdHandle(STD_INPUT_HANDLE);
  INPUT_RECORD rec;
  DWORD readCount = 0;
  while (*AbortEvent && PeekConsoleInput(stdIn, &rec, 1, &readCount) && readCount != 0)
  {
    ReadConsoleInput(stdIn, &rec, 1, &readCount);
    if (rec.EventType == KEY_EVENT && rec.Event.KeyEvent.wVirtualKeyCode == VK_ESCAPE && rec.Event.KeyEvent.bKeyDown)
    {
      SetEvent(*AbortEvent);
    }
  }
}

