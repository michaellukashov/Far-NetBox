//---------------------------------------------------------------------------
#include "Common.h"
#include "FileBuffer.h"
//---------------------------------------------------------------------------
char * EOLToStr(TEOLType EOLType)
{
  switch (EOLType) {
    case eolLF: return "\n";
    case eolCRLF: return "\r\n";
    case eolCR: return "\r";
    default: assert(false); return "";
  }
}
//---------------------------------------------------------------------------
TFileBuffer::TFileBuffer()
{
  FMemory = new TMemoryStream();
  FSize = 0;
}
//---------------------------------------------------------------------------
TFileBuffer::~TFileBuffer()
{
  delete FMemory;
}
//---------------------------------------------------------------------------
void TFileBuffer::SetSize(int value)
{
  if (FSize != value)
  {
    FMemory->Size = value;
    FSize = value;
  }
}
//---------------------------------------------------------------------------
void TFileBuffer::SetPosition(int value)
{
  FMemory->Position = value;
}
//---------------------------------------------------------------------------
int TFileBuffer::GetPosition() const
{
  return (int)FMemory->Position;
}
//---------------------------------------------------------------------------
void TFileBuffer::SetMemory(TMemoryStream * value)
{
  if (FMemory != value)
  {
    if (FMemory) delete FMemory;
    FMemory = value;
  }
}
//---------------------------------------------------------------------------
DWORD TFileBuffer::ReadStream(TStream * Stream, const DWORD Len, bool ForceLen)
{
  DWORD Result;
  try
  {
    Size = Position + Len;
    // C++5
    // FMemory->SetSize(FMemory->Position + Len);
    if (ForceLen)
    {
      Stream->ReadBuffer(Data + Position, Len);
      Result = Len;
    }
    else
    {
      Result = Stream->Read(Data + Position, Len);
    }
    if (Result != Len)
    {
      Size = Size - Len + Result;
    }
    FMemory->Seek(Len, soFromCurrent);
  }
  catch(EReadError &)
  {
    RaiseLastOSError();
  }
  return Result;
}
//---------------------------------------------------------------------------
DWORD TFileBuffer::LoadStream(TStream * Stream, const DWORD Len, bool ForceLen)
{
  FMemory->Seek(0, soFromBeginning);
  return ReadStream(Stream, Len, ForceLen);
}
//---------------------------------------------------------------------------
void TFileBuffer::Convert(char * Source, char * Dest, int Params,
  bool & /*Token*/)
{
  assert(strlen(Source) <= 2);
  assert(strlen(Dest) <= 2);

  if (FLAGSET(Params, cpRemoveBOM) && (Size >= 3) &&
      (memcmp(Data, "\xEF\xBB\xBF", 3) == 0))
  {
    Delete(0, 3);
  }

  if (FLAGSET(Params, cpRemoveCtrlZ) && (Size > 0) && ((*(Data + Size - 1)) == '\x1A'))
  {
    Delete(Size-1, 1);
  }

  if (strcmp(Source, Dest) == 0)
  {
    return;
  }

  char * Ptr = Data;

  // one character source EOL
  if (!Source[1])
  {
    // Disabled, not worth risking it is not safe enough, for the bugfix release
    #if 0
    bool PrevToken = Token;
    Token = false;
    #endif

    for (int Index = 0; Index < Size; Index++)
    {
      // EOL already in wanted format, make sure to pass unmodified
      if ((Index < Size - 1) && (*Ptr == Dest[0]) && (*(Ptr+1) == Dest[1]))
      {
        Index++;
        Ptr++;
      }
      #if 0
      // last buffer ended with the first char of wanted EOL format,
      // which got expanded to wanted format.
      // now we got the second char, so get rid of it.
      else if ((Index == 0) && PrevToken && (*Ptr == Dest[1]))
      {
        Delete(Index, 1);
      }
      #endif
      else if (*Ptr == Source[0])
      {
        #if 0
        if ((*Ptr == Dest[0]) && (Index == Size - 1))
        {
          Token = true;
        }
        #endif

        *Ptr = Dest[0];
        if (Dest[1])
        {
          Insert(Index+1, Dest+1, 1);
          Index++;
          Ptr = Data + Index;
        }
      }
      Ptr++;
    }
  }
  // two character source EOL
  else
  {
    int Index;
    for (Index = 0; Index < Size - 1; Index++)
    {
      if ((*Ptr == Source[0]) && (*(Ptr+1) == Source[1]))
      {
        *Ptr = Dest[0];
        if (Dest[1])
        {
          *(Ptr+1) = Dest[1];
          Index++; Ptr++;
        }
        else
        {
          Delete(Index+1, 1);
          Ptr = Data + Index;
        }
      }
      Ptr++;
    }
    if ((Index < Size) && (*Ptr == Source[0]))
    {
      Delete(Index, 1);
    }
  }
}
//---------------------------------------------------------------------------
void TFileBuffer::Convert(TEOLType Source, TEOLType Dest, int Params,
  bool & Token)
{
  Convert(EOLToStr(Source), EOLToStr(Dest), Params, Token);
}
//---------------------------------------------------------------------------
void TFileBuffer::Convert(char * Source, TEOLType Dest, int Params,
  bool & Token)
{
  Convert(Source, EOLToStr(Dest), Params, Token);
}
//---------------------------------------------------------------------------
void TFileBuffer::Convert(TEOLType Source, char * Dest, int Params,
  bool & Token)
{
  Convert(EOLToStr(Source), Dest, Params, Token);
}
//---------------------------------------------------------------------------
void TFileBuffer::Insert(int Index, const char * Buf, int Len)
{
  Size += Len;
  memmove(Data + Index + Len, Data + Index, Size - Index - Len);
  memmove(Data + Index, Buf, Len);
}
//---------------------------------------------------------------------------
void TFileBuffer::Delete(int Index, int Len)
{
  memmove(Data + Index, Data + Index + Len, Size - Index - Len);
  Size -= Len;
}
//---------------------------------------------------------------------------
void TFileBuffer::WriteToStream(TStream * Stream, const DWORD Len)
{
  try
  {
    Stream->WriteBuffer(Data + Position, Len);
    FMemory->Seek(Len, soFromCurrent);
  }
  catch(EWriteError &)
  {
    RaiseLastOSError();
  }
}
//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
TSafeHandleStream::TSafeHandleStream(int AHandle) :
  THandleStream(AHandle)
{
}
//---------------------------------------------------------------------------
int TSafeHandleStream::Read(void * Buffer, int Count)
{
  int Result = FileRead(FHandle, Buffer, Count);
  if (Result == -1)
  {
    RaiseLastOSError();
  }
  return Result;
}
//---------------------------------------------------------------------------
int TSafeHandleStream::Write(const void * Buffer, int Count)
{
  int Result = FileWrite(FHandle, Buffer, Count);
  if (Result == -1)
  {
    RaiseLastOSError();
  }
  return Result;
};
