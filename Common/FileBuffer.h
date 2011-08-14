//---------------------------------------------------------------------------
#ifndef FileBufferH
#define FileBufferH

#include <classes.hpp>
//---------------------------------------------------------------------------
enum TEOLType { eolLF /* \n */, eolCRLF /* \r\n */, eolCR /* \r */ };
const int cpRemoveCtrlZ = 0x01;
const int cpRemoveBOM =   0x02;
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
  __property TMemoryStream * Memory  = { read=FMemory, write=SetMemory };
  __property char * Data = { read=GetData };
  __property int Size = { read=FSize, write=SetSize };
  __property int Position = { read=GetPosition, write=SetPosition };

private:
  TMemoryStream * FMemory;
  int FSize;

  void SetMemory(TMemoryStream * value);
  char * GetData() const { return (char *)FMemory->Memory; }
  void SetSize(int value);
  void SetPosition(int value);
  int GetPosition() const;
};
//---------------------------------------------------------------------------
class TSafeHandleStream : public THandleStream
{
public:
  TSafeHandleStream(int AHandle);
  virtual int Read(void * Buffer, int Count);
  virtual int Write(const void * Buffer, int Count);
};
//---------------------------------------------------------------------------
char * EOLToStr(TEOLType EOLType);
//---------------------------------------------------------------------------
#endif
