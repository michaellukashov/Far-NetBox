
#pragma once

#include <Classes.hpp>
//---------------------------------------------------------------------------
extern const wchar_t * EOLTypeNames;
enum TEOLType
{
  eolLF,    // \n
  eolCRLF,  // \r\n
  eolCR     // \r
};

constexpr intptr_t cpRemoveCtrlZ = 0x01;
constexpr intptr_t cpRemoveBOM   = 0x02;
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TFileBuffer : public TObject
{
NB_DISABLE_COPY(TFileBuffer)
public:
  TFileBuffer() noexcept;
  virtual ~TFileBuffer() noexcept;
  void Convert(char * Source, char * Dest, intptr_t Params, bool & Token);
  void Convert(TEOLType Source, TEOLType Dest, intptr_t Params, bool & Token);
  void Convert(char * Source, TEOLType Dest, intptr_t Params, bool & Token);
  void Convert(TEOLType Source, char * Dest, intptr_t Params, bool & Token);
  void Insert(int64_t Index, const char * Buf, int64_t Len);
  void Delete(int64_t Index, int64_t Len);
  int64_t LoadStream(TStream * Stream, const int64_t Len, bool ForceLen);
  int64_t ReadStream(TStream * Stream, const int64_t Len, bool ForceLen);
  void WriteToStream(TStream * Stream, const int64_t Len);
  __property TMemoryStream * Memory  = { read=FMemory, write=SetMemory };
  RWProperty<TMemoryStream *> Memory{nb::bind(&TFileBuffer::GetMemory, this), nb::bind(&TFileBuffer::SetMemory, this)};
  __property char * Data = { read=GetData };
  ROProperty<char *> Data{nb::bind(&TFileBuffer::GetData, this)};
  __property int Size = { read=FSize, write=SetSize };
  RWProperty<int64_t> Size{nb::bind(&TFileBuffer::GetSize, this), nb::bind(&TFileBuffer::SetSize, this)};
  __property int Position = { read=GetPosition, write=SetPosition };
  RWProperty<int64_t> Position{nb::bind(&TFileBuffer::GetPosition, this), nb::bind(&TFileBuffer::SetPosition, this)};

public:
  TMemoryStream * GetMemory() const { return FMemory.get(); }
  void SetMemory(TMemoryStream * Value);
  char * GetData() const { return static_cast<char *>(FMemory->GetMemory()); }
  int64_t GetSize() const { return FMemory->GetSize(); }
  void SetSize(int64_t Value);
  void SetPosition(int64_t Value);
  int64_t GetPosition() const;

private:
  std::unique_ptr<TMemoryStream> FMemory;
};
//---------------------------------------------------------------------------
char * EOLToStr(TEOLType EOLType);
//---------------------------------------------------------------------------
