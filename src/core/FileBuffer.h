//---------------------------------------------------------------------------
#ifndef FileBufferH
#define FileBufferH

#include "Classes.h"
//---------------------------------------------------------------------------
enum TEOLType { eolLF /* \n */, eolCRLF /* \r\n */, eolCR /* \r */ };
const int cpRemoveCtrlZ = 0x01;
const int cpRemoveBOM =   0x02;
//---------------------------------------------------------------------------
#ifndef _MSC_VER
class TStream;
class TMemoryStream;
#endif
//---------------------------------------------------------------------------
class TFileBuffer
{
public:
  /* __fastcall */ TFileBuffer();
  virtual /* __fastcall */ ~TFileBuffer();
  void __fastcall Convert(char * Source, char * Dest, int Params, bool & Token);
  void __fastcall Convert(TEOLType Source, TEOLType Dest, int Params, bool & Token);
  void __fastcall Convert(char * Source, TEOLType Dest, int Params, bool & Token);
  void __fastcall Convert(TEOLType Source, char * Dest, int Params, bool & Token);
  void __fastcall Insert(int Index, const char * Buf, int Len);
  void __fastcall Delete(int Index, int Len);
  DWORD __fastcall LoadStream(TStream * Stream, const DWORD Len, bool ForceLen);
  DWORD __fastcall ReadStream(TStream * Stream, const DWORD Len, bool ForceLen);
  void __fastcall WriteToStream(TStream * Stream, const DWORD Len);
#ifndef _MSC_VER
  __property TMemoryStream * Memory  = { read=FMemory, write=SetMemory };
  __property char * Data = { read=GetData };
  __property int Size = { read=FSize, write=SetSize };
  __property int Position = { read=GetPosition, write=SetPosition };
#else
  TMemoryStream * __fastcall GetMemory() { return FMemory; }
  __int64 __fastcall GetSize() { return FSize; }
#endif

private:
  TMemoryStream * FMemory;
  __int64 FSize;

public:
  void __fastcall SetMemory(TMemoryStream * value);
  char * __fastcall GetData() const { return static_cast<char *>(FMemory->GetMemory()); }
  void __fastcall SetSize(__int64 value);
  void __fastcall SetPosition(__int64 value);
  __int64 __fastcall GetPosition() const;
};

//---------------------------------------------------------------------------

class TSafeHandleStream : public THandleStream
{
public:
  explicit /* __fastcall */ TSafeHandleStream(THandle AHandle);
  virtual /* __fastcall */ ~TSafeHandleStream() {}
  virtual __int64 __fastcall Read(void * Buffer, __int64 Count);
  virtual __int64 __fastcall Write(const void * Buffer, __int64 Count);
};

//---------------------------------------------------------------------------
char * __fastcall EOLToStr(TEOLType EOLType);
//---------------------------------------------------------------------------
#endif
