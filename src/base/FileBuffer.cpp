
#include <vcl.h>

#include <Common.h>
#include <FileBuffer.h>

__removed #pragma package(smart_init)

const wchar_t * EOLTypeNames = L"LF;CRLF;CR";

char * EOLToStr(TEOLType EOLType)
{
  switch (EOLType)
  {
  case eolLF:
    return const_cast<char *>("\n");
  case eolCRLF:
    return const_cast<char *>("\r\n");
  case eolCR:
    return const_cast<char *>("\r");
  default:
    DebugFail();
    return const_cast<char *>("");
  }
}

TFileBuffer::TFileBuffer() noexcept :
  FMemory(std::make_unique<TMemoryStream>())
{
}

TFileBuffer::~TFileBuffer() noexcept
{
  FMemory.reset();
}

void TFileBuffer::SetSize(int64_t Value)
{
  if (FMemory->GetSize() != Value)
  {
    FMemory->SetSize(Value);
  }
}

void TFileBuffer::SetPosition(int64_t Value)
{
  FMemory->SetPosition(Value);
}

int64_t TFileBuffer::GetPosition() const
{
  return FMemory->GetPosition();
}

void TFileBuffer::SetMemory(TMemoryStream * Value)
{
  if (FMemory.get() != Value)
  {
    FMemory.reset(Value);
  }
}

void TFileBuffer::ProcessRead(DWORD Len, DWORD Result)
{
  if (Result != Len)
  {
    Size = Size - Len + Result;
  }
  FMemory->Seek(Result, TSeekOrigin::soFromCurrent);
}

int64_t TFileBuffer::ReadStream(TStream * Stream, const int64_t Len, bool ForceLen)
{
  int64_t Result = 0;
  try
  {
    SetSize(GetPosition() + Len);
    // C++5
    // FMemory->SetSize(FMemory->Position + Len);
    if (ForceLen)
    {
      Stream->ReadBuffer(GetData() + GetPosition(), Len);
      Result = Len;
    }
    else
    {
      Result = Stream->Read(GetData() + GetPosition(), Len);
    }
    ProcessRead(Len, Result);
  }
  catch (EReadError &)
  {
    ::RaiseLastOSError();
  }
  return Result;
}

int64_t TFileBuffer::LoadStream(TStream * Stream, const int64_t Len, bool ForceLen)
{
  auto const res = FMemory->Seek(0, TSeekOrigin::soFromBeginning);
  DebugAssert(res == 0);
  return ReadStream(Stream, Len, ForceLen);
}

DWORD TFileBuffer::LoadFromIn(TTransferInEvent OnTransferIn, TObject * Sender, int64_t Len)
{
  FMemory->Seek(0, TSeekOrigin::soFromBeginning);
  DebugAssert(Position() == 0);
  Size = Position() + Len;
  size_t Result = OnTransferIn(Sender, reinterpret_cast<unsigned char *>(Data()) + Position(), Len);
  ProcessRead(Len, Result);
  return Result;
}

void TFileBuffer::Convert(char * Source, char * Dest, int32_t Params,
  bool & Token)
{
  DebugAssert(NBChTraitsCRT<char>::SafeStringLen(Source) <= 2);
  DebugAssert(NBChTraitsCRT<char>::SafeStringLen(Dest) <= 2);

  const std::string Bom(CONST_BOM);
  if (FLAGSET(Params, cpRemoveBOM) && (GetSize() >= 3) &&
      (memcmp(GetData(), Bom.c_str(), Bom.size()) == 0))
  {
    Delete(0, 3);
  }

  if (FLAGSET(Params, cpRemoveCtrlZ) && (GetSize() > 0) && ((*(GetData() + GetSize() - 1)) == '\x1A'))
  {
    Delete(GetSize() - 1, 1);
  }

  if (strcmp(Source, Dest) == 0)
  {
    return;
  }

  char * Ptr = GetData();

  // one character source EOL
  if (!Source[1])
  {
    const bool PrevToken = Token;
    Token = false;

    for (int32_t Index = 0; Index < GetSize(); ++Index)
    {
      // EOL already in destination format, make sure to pass it unmodified
      if ((Index < GetSize() - 1) && (*Ptr == Dest[0]) && (*(Ptr + 1) == Dest[1]))
      {
        ++Index;
        Ptr++;
      }
      // last buffer ended with the first char of destination 2-char EOL format,
      // which got expanded to full destination format.
      // now we got the second char, so get rid of it.
      else if ((Index == 0) && PrevToken && (*Ptr == Dest[1]))
      {
        Delete(Index, 1);
      }
      // we are ending with the first char of destination 2-char EOL format,
      // append the second char and make sure we strip it from the next buffer, if any
      else if ((*Ptr == Dest[0]) && (Index == GetSize() - 1) && Dest[1])
      {
        Token = true;
        Insert(Index + 1, Dest + 1, 1);
        ++Index;
        Ptr = GetData() + Index;
      }
      else if (*Ptr == Source[0])
      {
        *Ptr = Dest[0];
        if (Dest[1])
        {
          Insert(Index + 1, Dest + 1, 1);
          ++Index;
          Ptr = GetData() + Index;
        }
      }
      Ptr++;
    }
  }
  // two character source EOL
  else
  {
    int32_t Index;
    for (Index = 0; Index < GetSize() - 1; ++Index)
    {
      if ((*Ptr == Source[0]) && (*(Ptr + 1) == Source[1]))
      {
        *Ptr = Dest[0];
        if (Dest[1])
        {
          *(Ptr + 1) = Dest[1];
          ++Index;
          ++Ptr;
        }
        else
        {
          Delete(Index + 1, 1);
          Ptr = GetData() + Index;
        }
      }
      Ptr++;
    }
    if ((Index < GetSize()) && (*Ptr == Source[0]))
    {
      Delete(Index, 1);
    }
  }
}

void TFileBuffer::Convert(TEOLType Source, TEOLType Dest, int32_t Params,
  bool & Token)
{
  Convert(EOLToStr(Source), EOLToStr(Dest), Params, Token);
}

void TFileBuffer::Convert(char * Source, TEOLType Dest, int32_t Params,
  bool & Token)
{
  Convert(Source, EOLToStr(Dest), Params, Token);
}

void TFileBuffer::Convert(TEOLType Source, char * Dest, int32_t Params,
  bool & Token)
{
  Convert(EOLToStr(Source), Dest, Params, Token);
}

void TFileBuffer::Insert(int64_t Index, const char * Buf, int64_t Len)
{
  SetSize(GetSize() + Len);
  memmove(GetData() + Index + Len, GetData() + Index, nb::ToSizeT(GetSize() - Index - Len));
  memmove(GetData() + Index, Buf, nb::ToSizeT(Len));
}

void TFileBuffer::Delete(int64_t Index, int64_t Len)
{
  memmove(GetData() + Index, GetData() + Index + Len, nb::ToSizeT(GetSize() - Index - Len));
  SetSize(GetSize() - Len);
}

void TFileBuffer::WriteToStream(TStream * Stream, const int64_t Len)
{
  DebugAssert(Stream);
  try
  {
    Stream->WriteBuffer(GetData() + GetPosition(), Len);
    const int64_t res = FMemory->Seek(Len, TSeekOrigin::soFromCurrent);
    DebugAssert(res >= Len);
  }
  catch (EWriteError &)
  {
    ::RaiseLastOSError();
  }
}

void TFileBuffer::WriteToOut(TTransferOutEvent OnTransferOut, TObject * Sender, const int64_t Len)
{
  OnTransferOut(Sender, reinterpret_cast<const unsigned char *>(Data()) + Position(), Len);
  FMemory->Seek(Len, TSeekOrigin::soFromCurrent);
}

#if 0
moved to Classes.cpp
TSafeHandleStream::TSafeHandleStream(THandle AHandle) noexcept :
  THandleStream(AHandle)
{
}

int64_t TSafeHandleStream::Read(void * Buffer, int64_t Count)
{
  int Result = FileRead(FHandle, Buffer, Count);
  if (Result == -1)
  {
    RaiseLastOSError();
  }
  return Result;
}

int64_t TSafeHandleStream::Write(const void * Buffer, int64_t Count)
{
  int Result = FileWrite(FHandle, Buffer, Count);
  if (Result == -1)
  {
    RaiseLastOSError();
  }
  return Result;
}

int TSafeHandleStream::Read(System::DynamicArray<System::Byte> Buffer, int Offset, int Count)
{
  DebugFail(); // untested
  int Result = FileRead(FHandle, Buffer, Offset, Count);
  if (Result == -1)
  {
    RaiseLastOSError();
  }
  return Result;
}

int TSafeHandleStream::Write(const System::DynamicArray<System::Byte> Buffer, int Offset, int Count)
{
  // This is invoked for example by TIniFileStorage::Flush
  int Result = FileWrite(FHandle, Buffer, Offset, Count);
  if (Result == -1)
  {
    RaiseLastOSError();
  }
  return Result;
}
#endif // #if 0
