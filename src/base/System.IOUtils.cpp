#include "System.IOUtils.hpp"
#include "Sysutils.hpp"

TFileStream * TFile::OpenRead(const UnicodeString & FileName)
{
  TFileStream * Result = TFileStream::Create(FileName, fmOpenRead || fmShareDenyWrite);
  return Result;
}

TFileStream * TFile::OpenWrite(const UnicodeString & FileName)
{
  TFileStream * Result = TFileStream::Create(FileName, fmOpenWrite);
  return Result;
}

TBytes TFile::ReadAllBytes(const UnicodeString & FileName)
{
  TBytes Result;
  const std::unique_ptr<TFileStream> FileStream(TFile::OpenRead(FileName));
  const int64_t Size = FileStream->GetSize();
  Result.resize(Size);
  FileStream->ReadBuffer(Result.data(), Size);
  return Result;
}

UnicodeString TFile::ReadAllText(const UnicodeString & FileName)
{
  UnicodeString Result;
  DWORD LastError = ERROR_SUCCESS;

  HANDLE FileHandle = ::CreateFileW(FileName.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING,
    FILE_ATTRIBUTE_NORMAL, nullptr);
  if (FileHandle == INVALID_HANDLE_VALUE)
  {
    LastError = ::GetLastError();
  }

  if (LastError != ERROR_SUCCESS)
    Result.Clear();

  const int64_t Len = ::FileSeek(FileHandle, 0, FILE_END);
  ::FileSeek(FileHandle, 0, FILE_BEGIN);
  const AnsiString Buffer(nb::ToInt32(Len), 0);
  const int64_t Res = ::FileRead(FileHandle, const_cast<void *>(static_cast<void const *>(Buffer.data())), Len);
  if (Res != -1)
  {
    Result = Buffer;
  }

  if (CheckHandle(FileHandle))
  {
    SAFE_CLOSE_HANDLE(FileHandle);
    FileHandle = INVALID_HANDLE_VALUE;
  }
  return Result;
}

void TFile::WriteAllText(const UnicodeString & FileName, const UnicodeString & Text)
{
  FILE * File = base::LocalOpenFileForWriting(FileName);
  if (File)
  {
    void const * Data = static_cast<void const *>(Text.data());
    const size_t Size = Text.GetLength();
    base::WriteAndFlush(File, Data, Size);
    ::fclose(File);
  }
}
