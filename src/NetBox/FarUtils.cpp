#include <vcl.h>
#pragma hdrstop

#include <Common.h>

#include "FarUtils.h"

bool CNBFile::OpenWrite(const wchar_t * fileName)
{
  DebugAssert(m_File == INVALID_HANDLE_VALUE);
  DebugAssert(fileName);
  m_LastError = ERROR_SUCCESS;

  m_File = ::CreateFile(fileName, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (m_File == INVALID_HANDLE_VALUE)
  {
    m_LastError = ::GetLastError();
  }
  return (m_LastError == ERROR_SUCCESS);
}

bool CNBFile::OpenRead(const wchar_t * fileName)
{
  DebugAssert(m_File == INVALID_HANDLE_VALUE);
  DebugAssert(fileName);
  m_LastError = ERROR_SUCCESS;

  m_File = ::CreateFile(fileName, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
  if (m_File == INVALID_HANDLE_VALUE)
  {
    m_LastError = ::GetLastError();
  }
  return (m_LastError == ERROR_SUCCESS);
}

bool CNBFile::Read(void * buff, size_t & buffSize)
{
  DebugAssert(m_File != INVALID_HANDLE_VALUE);
  m_LastError = ERROR_SUCCESS;

  DWORD bytesRead = static_cast<DWORD>(buffSize);
  if (!::ReadFile(m_File, buff, bytesRead, &bytesRead, nullptr))
  {
    m_LastError = ::GetLastError();
    buffSize = 0;
  }
  else
  {
    buffSize = static_cast<size_t>(bytesRead);
  }
  return (m_LastError == ERROR_SUCCESS);
}

bool CNBFile::Write(const void * buff, const size_t buffSize)
{
  DebugAssert(m_File != INVALID_HANDLE_VALUE);
  m_LastError = ERROR_SUCCESS;

  DWORD bytesWritten;
  if (!::WriteFile(m_File, buff, static_cast<DWORD>(buffSize), &bytesWritten, nullptr))
  {
    m_LastError = ::GetLastError();
  }
  return (m_LastError == ERROR_SUCCESS);
}

int64_t CNBFile::GetFileSize() const
{
  DebugAssert(m_File != INVALID_HANDLE_VALUE);
  m_LastError = ERROR_SUCCESS;

  LARGE_INTEGER fileSize;
  if (!::GetFileSizeEx(m_File, &fileSize))
  {
    m_LastError = ::GetLastError();
    return -1;
  }
  return fileSize.QuadPart;
}

void CNBFile::Close()
{
  if (m_File != INVALID_HANDLE_VALUE)
  {
    SAFE_CLOSE_HANDLE(m_File);
    m_File = INVALID_HANDLE_VALUE;
  }
}

DWORD CNBFile::LastError() const
{
  return m_LastError;
}

DWORD CNBFile::SaveFile(const wchar_t * fileName, const rde::vector<char> & fileContent)
{
  CNBFile f;
  if (f.OpenWrite(fileName) && !fileContent.empty())
  {
    f.Write(&fileContent[0], fileContent.size());
  }
  return f.LastError();
}

DWORD CNBFile::SaveFile(const wchar_t * fileName, const char * fileContent)
{
  DebugAssert(fileContent);
  CNBFile f;
  if (f.OpenWrite(fileName) && fileContent && *fileContent)
  {
    f.Write(fileContent, NBChTraitsCRT<char>::SafeStringLen(fileContent));
  }
  return f.LastError();
}

DWORD CNBFile::LoadFile(const wchar_t * fileName, rde::vector<char> & fileContent)
{
  fileContent.clear();

  CNBFile f;
  if (f.OpenRead(fileName))
  {
    const int64_t fs = f.GetFileSize();
    if (fs < 0)
    {
      return f.LastError();
    }
    if (fs == 0)
    {
      return ERROR_SUCCESS;
    }
    size_t s = static_cast<size_t>(fs);
    fileContent.resize(s);
    f.Read(&fileContent[0], s);
  }
  return f.LastError();
}

void FarWrapText(UnicodeString Text, TStrings * Result, intptr_t MaxWidth)
{
  size_t TabSize = 8;
  TStringList Lines;
  Lines.SetText(Text);
  TStringList WrappedLines;
  for (intptr_t Index = 0; Index < Lines.GetCount(); ++Index)
  {
    UnicodeString WrappedLine = Lines.GetString(Index);
    if (!WrappedLine.IsEmpty())
    {
      WrappedLine = ::ReplaceChar(WrappedLine, L'\'', L'\3');
      WrappedLine = ::ReplaceChar(WrappedLine, L'\"', L'\4');
      WrappedLine = ::WrapText(WrappedLine, MaxWidth);
      WrappedLine = ::ReplaceChar(WrappedLine, L'\3', L'\'');
      WrappedLine = ::ReplaceChar(WrappedLine, L'\4', L'\"');
      WrappedLines.SetText(WrappedLine);
      for (intptr_t WrappedIndex = 0; WrappedIndex < WrappedLines.GetCount(); ++WrappedIndex)
      {
        UnicodeString FullLine = WrappedLines.GetString(WrappedIndex);
        do
        {
          // WrapText does not wrap when not possible, enforce it
          // (it also does not wrap when the line is longer than maximum only
          // because of trailing dot or similar)
          UnicodeString Line = FullLine.SubString(1, MaxWidth);
          FullLine.Delete(1, MaxWidth);

          intptr_t P;
          while ((P = Line.Pos(L'\t')) > 0)
          {
            Line.Delete(P, 1);
            Line.Insert(::StringOfChar(' ',
                ((P / TabSize) + ((P % TabSize) > 0 ? 1 : 0)) * TabSize - P + 1),
              P);
          }
          Result->Add(Line);
        }
        while (!FullLine.IsEmpty());
      }
    }
    else
    {
      Result->Add(L"");
    }
  }
}

