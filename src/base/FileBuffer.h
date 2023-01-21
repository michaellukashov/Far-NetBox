
#pragma once

#include <Classes.hpp>

extern const wchar_t * EOLTypeNames;
enum TEOLType
{
  eolLF,    // \n
  eolCRLF,  // \r\n
  eolCR     // \r
};

constexpr int32_t cpRemoveCtrlZ = 0x01;
constexpr int32_t cpRemoveBOM   = 0x02;

typedef void (__closure *TTransferOutEvent)(TObject * Sender, const unsigned char * Data, size_t Len);
typedef size_t (__closure *TTransferInEvent)(TObject * Sender, unsigned char * Data, size_t Len);

class NB_CORE_EXPORT TFileBuffer : public TObject
{
NB_DISABLE_COPY(TFileBuffer)
public:
  TFileBuffer() noexcept;
  virtual ~TFileBuffer() noexcept;
  void Convert(char * Source, char * Dest, int32_t Params, bool & Token);
  void Convert(TEOLType Source, TEOLType Dest, int32_t Params, bool & Token);
  void Convert(char * Source, TEOLType Dest, int32_t Params, bool & Token);
  void Convert(TEOLType Source, char * Dest, int32_t Params, bool & Token);
  void Insert(int64_t Index, const char * Buf, int64_t Len);
  void Delete(int64_t Index, int64_t Len);
  int64_t LoadStream(TStream * Stream, const int64_t Len, bool ForceLen);
  int64_t ReadStream(TStream * Stream, const int64_t Len, bool ForceLen);
  DWORD LoadFromIn(TTransferInEvent OnTransferIn, TObject * Sender, DWORD Len);
  void WriteToStream(TStream * Stream, const int64_t Len);
  void WriteToOut(TTransferOutEvent OnTransferOut, TObject * Sender, const DWORD Len);
  __property TMemoryStream * Memory  = { read=FMemory, write=SetMemory };
  RWProperty<TMemoryStream *> Memory{nb::bind(&TFileBuffer::GetMemory, this), nb::bind(&TFileBuffer::SetMemory, this)};
  __property char * Data = { read=GetData };
  ROProperty<char *> Data{nb::bind(&TFileBuffer::GetData, this)};
  __property int Size = { read=FSize, write=SetSize };
  RWProperty<int64_t> Size{nb::bind(&TFileBuffer::GetSize, this), nb::bind(&TFileBuffer::SetSize, this)};
  __property int Position = { read=GetPosition, write=SetPosition };
  RWProperty<int64_t> Position{nb::bind(&TFileBuffer::GetPosition, this), nb::bind(&TFileBuffer::SetPosition, this)};

private:
  TMemoryStream * FMemory;
  int FSize;

  void SetMemory(TMemoryStream * value);
  char * GetData() const { return (char *)FMemory->Memory; }
  void SetSize(int value);
  void SetPosition(int value);
  int GetPosition() const;
  void ProcessRead(DWORD Len, DWORD Result);
};

class TSafeHandleStream : public THandleStream
{
public:
  TSafeHandleStream(int AHandle);
  virtual int Read(void * Buffer, int Count);
  virtual int Write(const void * Buffer, int Count);
  virtual int Read(System::DynamicArray<System::Byte> Buffer, int Offset, int Count);
  virtual int Write(const System::DynamicArray<System::Byte> Buffer, int Offset, int Count);
};

char * EOLToStr(TEOLType EOLType);

