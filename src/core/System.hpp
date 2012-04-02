#ifndef SystemHPP
#define SystemHPP

// #pragma delphiheader begin
// #pragma option push
#pragma warning(push, 1)
// #pragma option -w-      // All warnings off
// #pragma option -Vx      // Zero-length empty class member functions
#pragma pack(push,8)
#define BCC32_HAS_CLASSMETHODS  // Flag we'll use __classmethod
#define ANSISTRING_AS_TEMPLATE  // Flag AnsiStringT<CP> support
#define _DELPHI_STRING_UNICODE  // Flag we're Unicode Mode
#define WIDECHAR_IS_WCHAR

// #include <sysmac.h>	// system macros
namespace System
{
typedef signed char          Shortint;         // -128..127
typedef short                Smallint;         // -32768..32767
typedef unsigned char        Byte;             // 0..255
typedef unsigned short       Word;             // 0..65535
typedef unsigned long        DWord;            // 0..4294967295
typedef unsigned             LongWord;         // 0..4294967295
typedef void*                Pointer;          //
typedef Pointer*             PPointer;         //
typedef char                 AnsiChar;         //
typedef signed long          Longint;          // -2147483648..2147484647
typedef signed __int64       Largeint;         //
typedef unsigned int         Cardinal;         // 0..4294967295
typedef long double          Extended;         // 10 byte real
typedef float                Single;           // 4 byte real
typedef double               Double;           // 8 byte real

enum TTextLineBreakStyle { tlbsLF, tlbsCRLF };

#define RTL_DELPHIRETURN 
#define PACKAGE 
#define __fastcall

#if defined(ANSISTRING_AS_TEMPLATE)
    class RTL_DELPHIRETURN AnsiStringBase;
    template <unsigned short CP> class RTL_DELPHIRETURN AnsiStringT;
    typedef  AnsiStringT<0> AnsiString;
#else
    class RTL_DELPHIRETURN AnsiString;
#endif
    class RTL_DELPHIRETURN WideString;
    class RTL_DELPHIRETURN UnicodeString;

    template <unsigned char sz> class SmallStringBase;
    template <unsigned char sz> class SmallString;

    template<class T> class RTL_DELPHIRETURN DynamicArray;

    class RTL_DELPHIRETURN TDateTimeBase;
    class RTL_DELPHIRETURN TDateTime;

    // Helper template used for when Delphi functions return static arrays.
    // The .hpp file uses StaticArray<type, size> to express such return
    // types.
    template <typename T, int size> struct StaticArray
    {
        T data[size];

        int Size() const { return size; };

        const T& operator[](int index) const { return data[index]; }
        T& operator[](int index) { return data[index]; }

        operator const T*() const { return data; }
        operator T*() { return data; }
    };

    typedef bool                 Boolean;          //
    typedef int                  Integer;          // -2147483648..2147484647
#if defined(WIDECHAR_IS_WCHAR)
    typedef wchar_t              WideChar;         // Unicode character
#else
    typedef uint16_t             WideChar;
#endif
    /* Unfortunate name :( */
    typedef signed char          Shortint;         // -128..127
    typedef short                Smallint;         // -32768..32767
    typedef unsigned char        Byte;             // 0..255
    typedef unsigned short       Word;             // 0..65535
    typedef unsigned long        DWord;            // 0..4294967295
    typedef unsigned             LongWord;         // 0..4294967295
    typedef void*                Pointer;          //
    typedef Pointer*             PPointer;         //
    typedef char                 AnsiChar;         //
    typedef signed long          Longint;          // -2147483648..2147484647
    typedef signed __int64       Largeint;         //
    typedef unsigned int         Cardinal;         // 0..4294967295
    typedef long double          Extended;         // 10 byte real
    typedef float                Single;           // 4 byte real
    typedef double               Double;           // 8 byte real
    typedef char* const          Openstring;       // D16 string/D32 shortstring formalparm
    typedef void*                file;             //
    typedef void*                Text;             //
    typedef Text                 TextFile;         //
    typedef char*                PAnsiChar;        //
    typedef WideChar*            PWideChar;        //
    // Uppercase 'C' 'Char' maps to Delphi's native 'char'
    // type for backward compatibility. To avoid confusion
    // of 'char' and 'Char', it's best to use _DCHAR
#if defined(_DELPHI_STRING_UNICODE)
    typedef WideChar             Char;
    typedef PWideChar            PChar;            //
    typedef WideChar             _DCHAR;
    #define _D(__s)              L ## __s
#else
    typedef char                 Char;             // 0..255
    typedef PAnsiChar            PChar;            //
    typedef char                 _DCHAR;
    #define _D(__s)              __s
#endif

    typedef unsigned char        ByteBool;         //
    typedef unsigned short       WordBool;         //
    typedef Cardinal             LongBool;         //

#if defined(_DELPHI_STRING_UNICODE)
    typedef UnicodeString        String;           //
#else
    typedef AnsiString           String;           //
#endif
    typedef SmallStringBase<255> ShortStringBase;  //
    typedef SmallString<255>     ShortString;      //
    typedef ShortString*         PShortString;     //
    typedef AnsiString*          PAnsiString;      //
    typedef UnicodeString*       PUnicodeString;   //
} // namespace System

//-- user supplied -----------------------------------------------------------
namespace System
{
  typedef Shortint ShortInt;
  typedef Smallint SmallInt;
  typedef Longint LongInt;
}
namespace System
{
  // typedef UCS4Char* TUCS4CharArray;
}
namespace System
{
  typedef int* IntegerArray;
}
namespace System
{
  typedef void *PointerArray;
}
namespace System
{
  typedef PChar TPCharArray;
}
namespace System
{
  typedef int *PLongInt;
  typedef PLongInt PLongint;
}
namespace System
{
    typedef System::TDateTime TDate;
    typedef System::TDateTime TTime;
}
namespace System
{
    typedef struct PackageUnitEntry UnitEntryTable;
}
namespace System
{
    extern PACKAGE HINSTANCE MainInstance;
}
namespace System
{
  typedef void * (__fastcall * TSystemThreadFuncProc)(void *, void * );
  typedef void (__fastcall * TSystemThreadEndProc)(int);
}

namespace System
{
//-- type declarations -------------------------------------------------------
typedef ShortInt Int8;

typedef short Int16;

typedef int Int32;

typedef Byte UInt8;

typedef Word UInt16;

typedef unsigned UInt32;

// #pragma option push -b-
// enum System__1 { vcPrivate, vcProtected, vcPublic, vcPublished };
// #pragma option pop

// typedef TMetaClass* TInterfacedClass;

typedef WideChar UCS2Char;

typedef WideChar * PUCS2Char;

// typedef TUCS4CharArray *PUCS4CharArray;

typedef AnsiStringT<65001> UTF8String;

typedef UTF8String *PUTF8String;

typedef AnsiStringT<65535> RawByteString;

typedef RawByteString *PRawByteString;

typedef IntegerArray *PIntegerArray;

typedef PointerArray *PPointerArray;

typedef DynamicArray<int> TBoundArray;

typedef TPCharArray *PPCharArray;

typedef unsigned *PCardinal;

typedef Word *PWord;

typedef unsigned __int64 *PUInt64;

typedef double *PDate;

typedef unsigned *PError;

typedef WordBool *PWordBool;

// typedef _di_IInterface *PUnknown;

// typedef PUnknown *PPUnknown;

typedef WideChar * *PPWideChar;

typedef char * *PPAnsiChar;

// typedef Comp *PComp;

typedef TDateTime *PDateTime;

typedef Word TVarType;

typedef int TVarOp;

typedef unsigned TResourceHandle;

// #pragma pack(push,1)
#pragma warning(push, 1)
struct TFileRec
{
	
public:
	int Handle;
	Word Mode;
	Word Flags;
	union
	{
		struct 
		{
			unsigned BufSize;
			unsigned BufPos;
			unsigned BufEnd;
			char *BufPtr;
			void *OpenFunc;
			void *InOutFunc;
			void *FlushFunc;
			void *CloseFunc;
			StaticArray<Byte, 32> UserData;
			StaticArray<WideChar, 260> Name;
			
		};
		struct 
		{
			unsigned RecSize;
			
		};
		
	};
};
// #pragma pack(pop)
#pragma warning(pop)

typedef StaticArray<char, 128> TTextBuf;

typedef TTextBuf *PTextBuf;

// #pragma pack(push,1)
#pragma warning(push, 1)
struct TTextRec
{
	
public:
	int Handle;
	Word Mode;
	Word Flags;
	unsigned BufSize;
	unsigned BufPos;
	unsigned BufEnd;
	char *BufPtr;
	void *OpenFunc;
	void *InOutFunc;
	void *FlushFunc;
	void *CloseFunc;
	StaticArray<Byte, 32> UserData;
	StaticArray<WideChar, 260> Name;
	TTextBuf Buffer;
};
// #pragma pack(pop)
#pragma warning(pop)

// typedef int __fastcall (*TTextIOFunc)(TTextRec &F);

// typedef int __fastcall (*TFileIOFunc)(TFileRec &F);

//-- var, const, procedure ---------------------------------------------------
static const bool False = false;
static const bool True = true;
static const int MaxInt = 2147483647;
static const int MaxLongint = 2147483647;
#define RTLVersion  (2.100000E+01)
static const ShortInt varEmpty = 0x0;
static const ShortInt varNull = 0x1;
static const ShortInt varSmallint = 0x2;
static const ShortInt varInteger = 0x3;
static const ShortInt varSingle = 0x4;
static const ShortInt varDouble = 0x5;
static const ShortInt varCurrency = 0x6;
static const ShortInt varDate = 0x7;
static const ShortInt varOleStr = 0x8;
static const ShortInt varDispatch = 0x9;
static const ShortInt varError = 0xa;
static const ShortInt varBoolean = 0xb;
static const ShortInt varVariant = 0xc;
static const ShortInt varUnknown = 0xd;
static const ShortInt varShortInt = 0x10;
static const ShortInt varByte = 0x11;
static const ShortInt varWord = 0x12;
static const ShortInt varLongWord = 0x13;
static const ShortInt varInt64 = 0x14;
static const ShortInt varUInt64 = 0x15;
static const ShortInt varStrArg = 0x48;
static const Word varString = 0x100;
static const Word varAny = 0x101;
static const Word varUString = 0x102;
static const Word varTypeMask = 0xfff;
static const Word varArray = 0x2000;
static const Word varByRef = 0x4000;
static const ShortInt vtInteger = 0x0;
static const ShortInt vtBoolean = 0x1;
static const ShortInt vtChar = 0x2;
static const ShortInt vtExtended = 0x3;
static const ShortInt vtString = 0x4;
static const ShortInt vtPointer = 0x5;
static const ShortInt vtPChar = 0x6;
static const ShortInt vtObject = 0x7;
static const ShortInt vtClass = 0x8;
static const ShortInt vtWideChar = 0x9;
static const ShortInt vtPWideChar = 0xa;
static const ShortInt vtAnsiString = 0xb;
static const ShortInt vtCurrency = 0xc;
static const ShortInt vtVariant = 0xd;
static const ShortInt vtInterface = 0xe;
static const ShortInt vtWideString = 0xf;
static const ShortInt vtInt64 = 0x10;
static const ShortInt vtUnicodeString = 0x11;
static const ShortInt vmtSelfPtr = -88;
static const ShortInt vmtIntfTable = -84;
static const ShortInt vmtAutoTable = -80;
static const ShortInt vmtInitTable = -76;
static const ShortInt vmtTypeInfo = -72;
static const ShortInt vmtFieldTable = -68;
static const ShortInt vmtMethodTable = -64;
static const ShortInt vmtDynamicTable = -60;
static const ShortInt vmtClassName = -56;
static const ShortInt vmtInstanceSize = -52;
static const ShortInt vmtParent = -48;
static const ShortInt hfFieldSize = 0x4;
static const ShortInt hfMonitorOffset = 0x0;
static const ShortInt opAdd = 0x0;
static const ShortInt opSubtract = 0x1;
static const ShortInt opMultiply = 0x2;
static const ShortInt opDivide = 0x3;
static const ShortInt opIntDivide = 0x4;
static const ShortInt opModulus = 0x5;
static const ShortInt opShiftLeft = 0x6;
static const ShortInt opShiftRight = 0x7;
static const ShortInt opAnd = 0x8;
static const ShortInt opOr = 0x9;
static const ShortInt opXor = 0xa;
static const ShortInt opCompare = 0xb;
static const ShortInt opNegate = 0xc;
static const ShortInt opNot = 0xd;
static const ShortInt opCmpEQ = 0xe;
static const ShortInt opCmpNE = 0xf;
static const ShortInt opCmpLT = 0x10;
static const ShortInt opCmpLE = 0x11;
static const ShortInt opCmpGT = 0x12;
static const ShortInt opCmpGE = 0x13;
static const ShortInt NumSmallBlockTypes = 0x37;
extern PACKAGE void *DispCallByIDProc;
extern PACKAGE void *ExceptProc;
// extern PACKAGE void __fastcall (*ErrorProc)(Byte ErrorCode, void * ErrorAddr);
extern PACKAGE void *ExceptClsProc;
extern PACKAGE void *ExceptObjProc;
extern PACKAGE void *RaiseExceptionProc;
extern PACKAGE void *RTLUnwindProc;
extern PACKAGE void *RaiseExceptObjProc;
extern PACKAGE void *ExceptionAcquired;
// extern PACKAGE TClass ExceptionClass;
// extern PACKAGE TSafeCallErrorProc SafeCallErrorProc;
// extern PACKAGE TAssertErrorProc AssertErrorProc;
// extern PACKAGE void __fastcall (*ExitProcessProc)(void);
// extern PACKAGE void __fastcall (*AbstractErrorProc)(void);
// extern PACKAGE unsigned HPrevInst [[deprecated]];
extern PACKAGE unsigned MainThreadID;
extern PACKAGE bool IsLibrary;
extern PACKAGE int CmdShow;
extern PACKAGE WideChar *CmdLine;
extern PACKAGE void *InitProc;
extern PACKAGE int ExitCode;
extern PACKAGE void *ExitProc;
extern PACKAGE void *ErrorAddr;
extern PACKAGE int RandSeed;
extern PACKAGE bool IsConsole;
extern PACKAGE bool IsMultiThread;
extern PACKAGE Byte FileMode;
extern PACKAGE Byte Test8086;
extern PACKAGE Byte Test8087;
extern PACKAGE ShortInt TestFDIV;
extern PACKAGE int CPUCount;
extern PACKAGE TextFile Input;
extern PACKAGE TextFile Output;
extern PACKAGE TextFile ErrOutput;
extern PACKAGE WideChar * *envp;
// extern PACKAGE void __fastcall (*VarClearProc)(TVarData &v);
// extern PACKAGE void __fastcall (*VarAddRefProc)(TVarData &v);
// extern PACKAGE void __fastcall (*VarCopyProc)(TVarData &Dest, const TVarData &Source);
// extern PACKAGE void __fastcall (*VarToLStrProc)(AnsiString &Dest, const TVarData &Source);
// extern PACKAGE void __fastcall (*VarToWStrProc)(WideString &Dest, const TVarData &Source);
static const ShortInt CPUi386 = 0x2;
static const ShortInt CPUi486 = 0x3;
static const ShortInt CPUPentium = 0x4;
extern PACKAGE Word Default8087CW;
extern PACKAGE Word HeapAllocFlags;
extern PACKAGE Byte DebugHook;
extern PACKAGE Byte JITEnable;
extern PACKAGE bool NoErrMsg;
extern PACKAGE int DefaultSystemCodePage;
extern PACKAGE int DefaultUnicodeCodePage;
extern PACKAGE unsigned UTF8CompareLocale;
extern PACKAGE TTextLineBreakStyle DefaultTextLineBreakStyle;
// #define sLineBreak "\r\n"
// extern PACKAGE int AllocMemCount [[deprecated]];
// extern PACKAGE int AllocMemSize [[deprecated]];
extern PACKAGE bool ReportMemoryLeaksOnShutdown;
extern PACKAGE bool NeverSleepOnMMThreadContention;
extern PACKAGE TSystemThreadFuncProc SystemThreadFuncProc;
extern PACKAGE TSystemThreadEndProc SystemThreadEndProc;
static const Word fmClosed = 0xd7b0;
static const Word fmInput = 0xd7b1;
static const Word fmOutput = 0xd7b2;
static const Word fmInOut = 0xd7b3;
static const ShortInt tfCRLF = 0x1;
// extern PACKAGE TLibModule *LibModuleList;
// extern PACKAGE TModuleUnloadRec *ModuleUnloadList;
// extern PACKAGE void *UnloadDelayLoadedDLLPtr;
// extern PACKAGE void *DelayLoadHelper;
// extern PACKAGE void *pfnDliNotifyHook;
// extern PACKAGE void *pfnDliFailureHook;
// extern PACKAGE void __fastcall TextStart [[deprecated]](void);
extern "C" void __stdcall SetLastError(int ErrorCode);
extern PACKAGE void * __fastcall SysGetMem(int Size);
extern PACKAGE int __fastcall SysFreeMem(void * P);
extern PACKAGE void * __fastcall SysReallocMem(void * P, int Size);
extern PACKAGE void * __fastcall SysAllocMem(unsigned Size);
// extern PACKAGE bool __fastcall SysRegisterExpectedMemoryLeak(void * P);
// extern PACKAGE bool __fastcall SysUnregisterExpectedMemoryLeak(void * P);
// extern PACKAGE void __fastcall GetMemoryManagerState(TMemoryManagerState &AMemoryManagerState);
// extern PACKAGE void __fastcall GetMemoryMap(TChunkStatus *AMemoryMap);
// extern PACKAGE THeapStatus __fastcall GetHeapStatus [[deprecated]](void);
// extern PACKAGE bool __fastcall AttemptToUseSharedMemoryManager(void);
// extern PACKAGE bool __fastcall ShareMemoryManager(void);
// extern PACKAGE TMinimumBlockAlignment __fastcall GetMinimumBlockAlignment(void);
// extern PACKAGE void __fastcall SetMinimumBlockAlignment(TMinimumBlockAlignment AMinimumBlockAlignment);
extern PACKAGE void * __fastcall AllocMem(unsigned Size);
// extern PACKAGE bool __fastcall RegisterExpectedMemoryLeak(void * P);
// extern PACKAGE bool __fastcall UnregisterExpectedMemoryLeak(void * P);
// extern PACKAGE void __fastcall GetMemoryManager [[deprecated]](TMemoryManager &MemMgr)/* overload */;
// extern PACKAGE void __fastcall SetMemoryManager [[deprecated]](const TMemoryManager &MemMgr)/* overload */;
// extern PACKAGE void __fastcall GetMemoryManager(TMemoryManagerEx &MemMgrEx)/* overload */;
// extern PACKAGE void __fastcall SetMemoryManager(const TMemoryManagerEx &MemMgrEx)/* overload */;
// extern PACKAGE bool __fastcall IsMemoryManagerSet(void);
// extern PACKAGE TObject* __fastcall ExceptObject(void);
// extern PACKAGE void * __fastcall ExceptAddr(void);
// extern PACKAGE void * __fastcall AcquireExceptionObject(void);
// extern PACKAGE void __fastcall ReleaseExceptionObject(void);
// extern PACKAGE void * __fastcall RaiseList [[deprecated("Use AcquireExceptionObject")]](void);
// extern PACKAGE void * __fastcall SetRaiseList [[deprecated("Use AcquireExceptionObject")]](void * NewPtr);
// extern PACKAGE void __fastcall SetInOutRes(int NewValue);
extern PACKAGE void __fastcall ChDir(const UnicodeString S)/* overload */;
extern PACKAGE void __fastcall ChDir(WideChar * P)/* overload */;
extern PACKAGE int __fastcall IOResult(void);
extern PACKAGE void __fastcall MkDir(const UnicodeString S)/* overload */;
extern PACKAGE void __fastcall MkDir(WideChar * P)/* overload */;
extern PACKAGE void __fastcall Move(const void *Source, void *Dest, int Count);
extern PACKAGE void __fastcall MoveChars(const void *Source, void *Dest, int Length);
extern PACKAGE int __fastcall ParamCount(void);
extern PACKAGE UnicodeString __fastcall ParamStr(int Index);
extern PACKAGE void __fastcall Randomize(void);
extern PACKAGE int __fastcall Random(const int ARange)/* overload */;
extern PACKAGE Extended __fastcall Random(void)/* overload */;
extern PACKAGE void __fastcall RmDir(const UnicodeString S)/* overload */;
extern PACKAGE void __fastcall RmDir(WideChar * P)/* overload */;
extern PACKAGE char __fastcall UpCase(char Ch)/* overload */;
extern PACKAGE WideChar __fastcall UpCase(WideChar Ch)/* overload */;
extern PACKAGE void __fastcall Set8087CW(Word NewCW);
extern PACKAGE Word __fastcall Get8087CW(void);
extern PACKAGE Extended __fastcall Int(const Extended X);
extern PACKAGE Extended __fastcall Frac(const Extended X);
extern PACKAGE Extended __fastcall Exp(const Extended X);
extern PACKAGE Extended __fastcall Cos(const Extended X);
extern PACKAGE Extended __fastcall Sin(const Extended X);
extern PACKAGE Extended __fastcall Ln(const Extended X);
extern PACKAGE Extended __fastcall ArcTan(const Extended X);
extern PACKAGE Extended __fastcall Sqrt(const Extended X);
// extern PACKAGE int __fastcall Flush(TextFile &t);
// extern PACKAGE void __fastcall Mark [[deprecated]](void);
// extern PACKAGE void __fastcall Release [[deprecated]](void);
// extern PACKAGE void __fastcall FPower10(void);
// extern PACKAGE void * __fastcall GetDynaMethod(TClass vmt, short selector);
// extern PACKAGE bool __fastcall MonitorEnter(TObject* AObject, unsigned Timeout = (unsigned)(0xffffffff));
// extern PACKAGE bool __fastcall MonitorTryEnter(TObject* AObject);
// extern PACKAGE void __fastcall MonitorExit(TObject* AObject);
// extern PACKAGE bool __fastcall MonitorWait(TObject* AObject, unsigned Timeout);
// extern PACKAGE void __fastcall MonitorPulse(TObject* AObject);
// extern PACKAGE void __fastcall MonitorPulseAll(TObject* AObject);
// extern PACKAGE void __fastcall MemoryBarrier(void);
// extern PACKAGE int __fastcall BeginThread(void * SecurityAttributes, unsigned StackSize, TThreadFunc ThreadFunc, void * Parameter, unsigned CreationFlags, unsigned &ThreadId);
// extern PACKAGE void __fastcall EndThread(int ExitCode);
extern PACKAGE Word __fastcall StringElementSize(const UnicodeString S)/* overload */;
extern PACKAGE Word __fastcall StringElementSize(const RawByteString S)/* overload */;
extern PACKAGE Word __fastcall StringCodePage(const UnicodeString S)/* overload */;
extern PACKAGE Word __fastcall StringCodePage(const RawByteString S)/* overload */;
extern PACKAGE int __fastcall StringRefCount(const UnicodeString S)/* overload */;
extern PACKAGE int __fastcall StringRefCount(const RawByteString S)/* overload */;
extern PACKAGE void __fastcall SetAnsiString(PAnsiString Dest, WideChar * Source, int Length, Word CodePage);
extern PACKAGE void __fastcall UniqueString(AnsiString &str)/* overload */;
extern PACKAGE void __fastcall UniqueString(WideString &str)/* overload */;
extern PACKAGE int __fastcall Pos(const RawByteString substr, const RawByteString str)/* overload */;
extern PACKAGE AnsiString __fastcall StringOfChar(char ch, int Count)/* overload */;
extern PACKAGE int __fastcall Pos(const WideString substr, const WideString str)/* overload */;
extern PACKAGE UnicodeString __fastcall StringOfChar(WideChar ch, int Count)/* overload */;
// extern PACKAGE UCS4String __fastcall UnicodeStringToUCS4String(const UnicodeString S);
// extern PACKAGE UnicodeString __fastcall UCS4StringToUnicodeString(const UCS4String S);
extern PACKAGE void __fastcall UniqueString(UnicodeString &str)/* overload */;
extern PACKAGE int __fastcall Pos(const UnicodeString substr, const UnicodeString str)/* overload */;
extern PACKAGE void __fastcall SetCodePage(RawByteString &S, Word CodePage, bool Convert = true);
// extern PACKAGE void __fastcall InitializeArray(void * p, void * typeInfo, unsigned elemCount);
// extern PACKAGE UnicodeString __fastcall WideCharToString(WideChar * Source);
// extern PACKAGE UnicodeString __fastcall WideCharLenToString(WideChar * Source, int SourceLen);
// extern PACKAGE void __fastcall WideCharToStrVar(WideChar * Source, UnicodeString &Dest);
// extern PACKAGE void __fastcall WideCharLenToStrVar(WideChar * Source, int SourceLen, UnicodeString &Dest)/* overload */;
// extern PACKAGE void __fastcall WideCharLenToStrVar(WideChar * Source, int SourceLen, AnsiString &Dest)/* overload */;
extern PACKAGE WideChar * __fastcall StringToWideChar(const UnicodeString Source, WideChar * Dest, int DestSize);
// extern PACKAGE UnicodeString __fastcall OleStrToString(WideChar * Source);
// extern PACKAGE void __fastcall OleStrToStrVar(WideChar * Source, AnsiString &Dest)/* overload */;
// extern PACKAGE void __fastcall OleStrToStrVar(WideChar * Source, UnicodeString &Dest)/* overload */;
// extern PACKAGE WideChar * __fastcall StringToOleStr(const AnsiString Source)/* overload */;
// extern PACKAGE WideChar * __fastcall StringToOleStr(const UnicodeString Source)/* overload */;
// extern PACKAGE void __fastcall GetVariantManager [[deprecated]](TVariantManager &VarMgr);
// extern PACKAGE void __fastcall SetVariantManager [[deprecated]](const TVariantManager &VarMgr);
// extern PACKAGE bool __fastcall IsVariantManagerSet [[deprecated]](void);
// extern PACKAGE void __fastcall CopyArray(void * dest, void * source, void * typeInfo, int cnt);
// extern PACKAGE void __fastcall FinalizeArray(void * p, void * typeInfo, unsigned cnt);
// extern PACKAGE void __fastcall DynArrayClear(void * &a, void * typeInfo);
// extern PACKAGE void __fastcall DynArraySetLength(void * &a, void * typeInfo, int dimCnt, PLongint lengthVec);
// extern PACKAGE int __fastcall DynArraySize(void * a);
extern PACKAGE unsigned __fastcall FindHInstance(void * Address);
// extern PACKAGE unsigned __fastcall FindClassHInstance(TClass ClassType);
extern PACKAGE unsigned __fastcall FindResourceHInstance(unsigned Instance);
extern PACKAGE UnicodeString __fastcall GetUILanguages(const Word LANGID);
extern PACKAGE UnicodeString __fastcall GetLocaleOverride(UnicodeString AppName);
extern PACKAGE unsigned __fastcall LoadResourceModule(WideChar * ModuleName, bool CheckOwner = true);
// extern PACKAGE UnicodeString __fastcall GetResourceModuleName(UnicodeString HostAppName, UnicodeString ModuleName);
// extern PACKAGE void __fastcall EnumModules(TEnumModuleFunc Func, void * Data)/* overload */;
// extern PACKAGE void __fastcall EnumResourceModules(TEnumModuleFunc Func, void * Data)/* overload */;
// extern PACKAGE void __fastcall EnumModules(TEnumModuleFuncLW Func, void * Data)/* overload */;
// extern PACKAGE void __fastcall EnumResourceModules(TEnumModuleFuncLW Func, void * Data)/* overload */;
// extern PACKAGE void __fastcall AddModuleUnloadProc(TModuleUnloadProc Proc)/* overload */;
// extern PACKAGE void __fastcall RemoveModuleUnloadProc(TModuleUnloadProc Proc)/* overload */;
// extern PACKAGE void __fastcall AddModuleUnloadProc(TModuleUnloadProcLW Proc)/* overload */;
// extern PACKAGE void __fastcall RemoveModuleUnloadProc(TModuleUnloadProcLW Proc)/* overload */;
// extern PACKAGE void __fastcall RegisterModule(PLibModule LibModule);
// extern PACKAGE void __fastcall UnregisterModule(PLibModule LibModule);
// extern PACKAGE double __cdecl CompToDouble(Comp Value);
// extern PACKAGE void __cdecl DoubleToComp(double Value, Comp &Result);
// extern PACKAGE Currency __cdecl CompToCurrency(Comp Value);
// extern PACKAGE void __cdecl CurrencyToComp(Currency Value, Comp &Result);
extern PACKAGE void * __cdecl GetMemory(int Size);
extern PACKAGE int __cdecl FreeMemory(void * P);
extern PACKAGE void * __cdecl ReallocMemory(void * P, int Size);
// extern PACKAGE void __fastcall SetLineBreakStyle(TextFile &T, TTextLineBreakStyle Style);
// extern PACKAGE int __fastcall UnicodeToUtf8 [[deprecated]](char * Dest, WideChar * Source, int MaxBytes)/* overload */;
extern PACKAGE unsigned __fastcall UnicodeToUtf8(char * Dest, unsigned MaxDestBytes, WideChar * Source, unsigned SourceChars)/* overload */;
// extern PACKAGE int __fastcall Utf8ToUnicode [[deprecated]](WideChar * Dest, char * Source, int MaxChars)/* overload */;
extern PACKAGE unsigned __fastcall Utf8ToUnicode(WideChar * Dest, unsigned MaxDestChars, char * Source, unsigned SourceBytes)/* overload */;
extern PACKAGE RawByteString __fastcall UTF8Encode(const WideString WS)/* overload */;
extern PACKAGE RawByteString __fastcall UTF8Encode(const UnicodeString US)/* overload */;
extern PACKAGE RawByteString __fastcall UTF8Encode(const RawByteString A)/* overload */;
extern PACKAGE ShortString __fastcall UTF8EncodeToShortString(const WideString WS)/* overload */;
extern PACKAGE ShortString __fastcall UTF8EncodeToShortString(const UnicodeString US)/* overload */;
extern PACKAGE ShortString __fastcall UTF8EncodeToShortString(const RawByteString A)/* overload */;
// extern PACKAGE WideString __fastcall UTF8Decode [[deprecated("Use UTF8ToWideString or UTF8ToString")]](const RawByteString S);
extern PACKAGE WideString __fastcall UTF8ToWideString(const RawByteString S);
extern PACKAGE UnicodeString __fastcall UTF8ToUnicodeString(const RawByteString S)/* overload */;
extern PACKAGE UnicodeString __fastcall UTF8ToUnicodeString(const char * S)/* overload */;
extern PACKAGE UnicodeString __fastcall UTF8ToUnicodeString(const ShortString &S)/* overload */;
extern PACKAGE UnicodeString __fastcall UTF8ToString(const RawByteString S)/* overload */;
extern PACKAGE UnicodeString __fastcall UTF8ToString(const ShortString &S)/* overload */;
extern PACKAGE UnicodeString __fastcall UTF8ToString(const char * S)/* overload */;
extern PACKAGE UnicodeString __fastcall UTF8ToString(char const *S, const int S_Size)/* overload */;
extern PACKAGE RawByteString __fastcall AnsiToUtf8(const UnicodeString S);
extern PACKAGE UnicodeString __fastcall Utf8ToAnsi(const RawByteString S);
// extern PACKAGE UnicodeString __fastcall LoadResString(PResStringRec ResStringRec);
// extern PACKAGE PUCS4Char __fastcall PUCS4Chars(const UCS4String S);
// extern PACKAGE UCS4String __fastcall WideStringToUCS4String(const WideString S);
// extern PACKAGE WideString __fastcall UCS4StringToWideString(const UCS4String S);
// extern PACKAGE void __fastcall SetMultiByteConversionCodePage(int CodePage);
// extern PACKAGE void __stdcall UnloadDelayLoadedDLL(char * szDll);

}	/* namespace System */
// using namespace System;
// #include <sysclass.h>	// system class definitions
// #pragma pack(pop)
// #pragma option pop
#pragma warning(pop)


// #pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// SystemHPP
