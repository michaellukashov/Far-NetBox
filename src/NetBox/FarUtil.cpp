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
  size_t delimScheme = urlParse.Pos(L"://");
  if (delimScheme > 0)
  {
    if (scheme)
    {
      *scheme = urlParse.SubString(1, delimScheme);
      // transform(scheme->begin(), scheme->end(), scheme->begin(), tolower);
      scheme->Lower();
    }
    urlParse.Delete(1, delimScheme + sizeof(L"://") / sizeof(wchar_t) - 1);
  }

  //Parse path
  const size_t delimPath = urlParse.Pos(L'/');
  if (delimPath > 0)
  {
    UnicodeString parsePath = urlParse.SubString(delimPath);
    urlParse.Delete(1, delimPath);
    //Parse query
    size_t delimQuery = 0;
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
  size_t delimLogin = 0;
  if (urlParse.RPos(delimLogin, L'@'))
  {
    UnicodeString parseLogin = urlParse.SubString(0, delimLogin);
    size_t delimPwd = 0;
    if (parseLogin.RPos(delimPwd, L':'))
    {
      if (password)
      {
        *password = parseLogin.SubString(delimPwd + 1);
      }
      parseLogin.Delete(1, delimPwd);
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

  size_t delimPort = 0;
  if (urlParse.RPos(delimPort, L':'))
  {
    if (port)
    {
      const UnicodeString portNum = urlParse.SubString(delimPort + 1);
      *port = static_cast<unsigned short>(_wtoi(portNum.c_str()));
    }
    urlParse.Delete(1, delimPort);
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

