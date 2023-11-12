
#pragma once

#include <Classes.hpp>

extern const wchar_t * EOLTypeNames;
enum TEOLType {
  eolLF,    // \n
  eolCRLF,  // \r\n
  eolCR     // \r
};

constexpr int32_t cpRemoveCtrlZ = 0x01;
constexpr int32_t cpRemoveBOM   = 0x02;

using TTransferOutEvent = nb::FastDelegate3<void, TObject * /*Sender*/, const uint8_t * /*Data*/, size_t /*Len*/>;
using TTransferInEvent = nb::FastDelegate3<size_t, TObject * /*Sender*/, uint8_t * /*Data*/, size_t /*Len*/>;

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
  DWORD LoadFromIn(TTransferInEvent OnTransferIn, TObject * Sender, int64_t Len);
  void WriteToStream(TStream * Stream, const int64_t Len);
  void WriteToOut(TTransferOutEvent OnTransferOut, TObject * Sender, const int64_t Len);
  void Reset();
  __property TMemoryStream * Memory  = { read=FMemory };
  ROProperty<TMemoryStream *> Memory{nb::bind(&TFileBuffer::GetMemory, this)};
  __property char * Data = { read=GetData };
  ROProperty<char *> Data{nb::bind(&TFileBuffer::GetData, this)};
  __property int32_t Size = { read=FSize, write=SetSize };
  RWProperty3<int64_t> Size{nb::bind(&TFileBuffer::GetSize, this), nb::bind(&TFileBuffer::SetSize, this)};

private:
  std::unique_ptr<TMemoryStream> FMemory;
  // int32_t FSize;

public:
  TMemoryStream * GetMemory() const { return FMemory.get(); }
  void SetMemory(TMemoryStream * value);
  char * GetData() const { return static_cast<char *>(FMemory->GetMemory()); }
  char * GetPointer() const { return GetData() + GetPosition(); }
  void NeedSpace(int64_t Size);
  int64_t GetSize() const { return FMemory->GetSize(); }
  void SetSize(int64_t Value);
  int32_t GetPosition() const { return nb::ToInt(FMemory->Position); }
  void SetPosition(int64_t Value);
  void ProcessRead(DWORD Len, DWORD Result);
};

#if 0
moved to Classes.h
class TSafeHandleStream : public THandleStream
{
public:
  TSafeHandleStream(int32_t AHandle);
  TSafeHandleStream(THandleStream * Source, bool Own);
  static TSafeHandleStream * CreateFromFile(const UnicodeString & FileName, uint16_t Mode);
  virtual ~TSafeHandleStream();
  virtual int32_t Read(void * Buffer, int32_t Count);
  virtual int32_t Write(const void * Buffer, int32_t Count);
  virtual int32_t Read(System::DynamicArray<System::Byte> Buffer, int32_t Offset, int32_t Count);
  virtual int32_t Write(const System::DynamicArray<System::Byte> Buffer, int32_t Offset, int32_t Count);
private:
  THandleStream * FSource;
  bool FOwned;
};
#endif // if 0

char * EOLToStr(TEOLType EOLType);

