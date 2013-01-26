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
  void Insert(__int64 Index, const char * Buf, size_t Len);
  void Delete(__int64 Index, size_t Len);
  __int64 LoadStream(TStream * Stream, const __int64 Len, bool ForceLen);
  __int64 ReadStream(TStream * Stream, const __int64 Len, bool ForceLen);
  void WriteToStream(TStream * Stream, const __int64 Len);

private:
  TMemoryStream * FMemory;

public:
  TMemoryStream * GetMemory() { return FMemory; }
  void SetMemory(TMemoryStream * Value);
  char * GetData() const { return static_cast<char *>(FMemory->GetMemory()); }
  __int64 GetSize() const { return FMemory->Size; }
  void SetSize(__int64 Value);
  void SetPosition(__int64 Value);
  __int64 GetPosition() const;

private:
  TFileBuffer(const TFileBuffer &);
  TFileBuffer & operator=(const TFileBuffer &);
};
//---------------------------------------------------------------------------
class TSafeHandleStream : public THandleStream
{
public:
  explicit TSafeHandleStream(THandle AHandle);
  virtual ~TSafeHandleStream() {}
  virtual __int64 Read(void * Buffer, __int64 Count);
  virtual __int64 Write(const void * Buffer, __int64 Count);
};
//---------------------------------------------------------------------------
char * EOLToStr(TEOLType EOLType);
//---------------------------------------------------------------------------
#endif
