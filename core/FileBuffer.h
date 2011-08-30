//---------------------------------------------------------------------------
#ifndef FileBufferH
#define FileBufferH

#include "Classes.h"
//---------------------------------------------------------------------------
enum TEOLType { eolLF /* \n */, eolCRLF /* \r\n */, eolCR /* \r */ };
const int cpRemoveCtrlZ = 0x01;
const int cpRemoveBOM =   0x02;
//---------------------------------------------------------------------------
class TStream;
class TMemoryStream;
//---------------------------------------------------------------------------
class TFileBuffer
{
public:
  TFileBuffer();
  virtual ~TFileBuffer();
  void Convert(char * Source, char * Dest, int Params, bool & Token);
  void Convert(TEOLType Source, TEOLType Dest, int Params, bool & Token);
  void Convert(char * Source, TEOLType Dest, int Params, bool & Token);
  void Convert(TEOLType Source, char * Dest, int Params, bool & Token);
  void Insert(int Index, const char * Buf, int Len);
  void Delete(int Index, int Len);
  DWORD LoadStream(TStream * Stream, const DWORD Len, bool ForceLen);
  DWORD ReadStream(TStream * Stream, const DWORD Len, bool ForceLen);
  void WriteToStream(TStream * Stream, const DWORD Len);
  // __property TMemoryStream * Memory  = { read=FMemory, write=SetMemory };
  TMemoryStream *GetMemory() { return FMemory; }
  void SetMemory(TMemoryStream * value);
  // __property char * Data = { read=GetData };
  char *GetData() const { return NULL; } // FIXME (char *)FMemory->Memory; }
  // __property int Size = { read=FSize, write=SetSize };
  int GetSize() { return FSize; }
  void SetSize(int value);
  // __property int Position = { read=GetPosition, write=SetPosition };
  int GetPosition() const;
  void SetPosition(int value);

private:
  TMemoryStream * FMemory;
  int FSize;

};

//---------------------------------------------------------------------------

class TSafeHandleStream : public THandleStream
{
public:
  TSafeHandleStream(HANDLE AHandle);
  virtual int Read(void * Buffer, int Count);
  virtual int Write(const void * Buffer, int Count);
};

//---------------------------------------------------------------------------
char * EOLToStr(TEOLType EOLType);
//---------------------------------------------------------------------------
#endif
