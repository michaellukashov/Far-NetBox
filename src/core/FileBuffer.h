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
    TFileBuffer();
    virtual ~TFileBuffer();
    void __fastcall Convert(char *Source, char *Dest, int Params, bool &Token);
    void __fastcall Convert(TEOLType Source, TEOLType Dest, int Params, bool &Token);
    void __fastcall Convert(char *Source, TEOLType Dest, int Params, bool &Token);
    void __fastcall Convert(TEOLType Source, char *Dest, int Params, bool &Token);
    void __fastcall Insert(size_t Index, const char *Buf, size_t Len);
    void __fastcall Delete(size_t Index, size_t Len);
    size_t __fastcall LoadStream(TStream *Stream, size_t Len, bool ForceLen);
    size_t __fastcall ReadStream(TStream *Stream, size_t Len, bool ForceLen);
    void __fastcall WriteToStream(TStream *Stream, size_t  Len);
    TMemoryStream * __fastcall GetMemory() { return FMemory; }
    void __fastcall SetMemory(TMemoryStream *value);
    char * __fastcall GetData() const { return static_cast<char *>(FMemory->GetMemory()); }
    __int64 __fastcall GetSize() { return FSize; }
    void __fastcall SetSize(__int64 value);
    __int64 __fastcall GetPosition() const;
    void __fastcall SetPosition(__int64 value);

private:
    TMemoryStream *FMemory;
    __int64 FSize;
};

//---------------------------------------------------------------------------

class TSafeHandleStream : public THandleStream
{
public:
    explicit TSafeHandleStream(HANDLE AHandle);
    virtual ~TSafeHandleStream();
    virtual __int64 __fastcall Read(void *Buffer, __int64 Count);
    virtual __int64 __fastcall Write(const void *Buffer, __int64 Count);
};

//---------------------------------------------------------------------------
char * __fastcall EOLToStr(TEOLType EOLType);
//---------------------------------------------------------------------------
#endif
