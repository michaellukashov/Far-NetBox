//---------------------------------------------------------------------------
#ifndef FileBufferH
#define FileBufferH

#include "Classes.h"
//---------------------------------------------------------------------------
enum TEOLType { eolLF /* \n */, eolCRLF /* \r\n */, eolCR /* \r */ };
const int cpRemoveCtrlZ = 0x01;
const int cpRemoveBOM =   0x02;
//---------------------------------------------------------------------------
class nb::TStream;
class nb::TMemoryStream;
//---------------------------------------------------------------------------
class TFileBuffer
{
public:
    TFileBuffer();
    virtual ~TFileBuffer();
    void Convert(char *Source, char *Dest, int Params, bool &Token);
    void Convert(TEOLType Source, TEOLType Dest, int Params, bool &Token);
    void Convert(char *Source, TEOLType Dest, int Params, bool &Token);
    void Convert(TEOLType Source, char *Dest, int Params, bool &Token);
    void Insert(size_t Index, const char *Buf, size_t Len);
    void Delete(size_t Index, size_t Len);
    size_t LoadStream(nb::TStream *Stream, size_t Len, bool ForceLen);
    size_t ReadStream(nb::TStream *Stream, size_t Len, bool ForceLen);
    void WriteToStream(nb::TStream *Stream, size_t  Len);
    // __property nb::TMemoryStream * Memory  = { read=FMemory, write=SetMemory };
    nb::TMemoryStream *GetMemory() { return FMemory; }
    void SetMemory(nb::TMemoryStream *value);
    // __property char * Data = { read=GetData };
    char *GetData() const { return static_cast<char *>(FMemory->GetMemory()); }
    // __property int Size = { read=FSize, write=SetSize };
    __int64 GetSize() { return FSize; }
    void SetSize(__int64 value);
    // __property int Position = { read=GetPosition, write=SetPosition };
    __int64 GetPosition() const;
    void SetPosition(__int64 value);

private:
    nb::TMemoryStream *FMemory;
    __int64 FSize;
};

//---------------------------------------------------------------------------

class TSafeHandleStream : public nb::THandleStream
{
public:
    explicit TSafeHandleStream(HANDLE AHandle);
    virtual ~TSafeHandleStream();
    virtual __int64 Read(void *Buffer, __int64 Count);
    virtual __int64 Write(const void *Buffer, __int64 Count);
};

//---------------------------------------------------------------------------
char *EOLToStr(TEOLType EOLType);
//---------------------------------------------------------------------------
#endif
