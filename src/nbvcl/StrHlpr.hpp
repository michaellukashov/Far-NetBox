// CodeGear C++Builder
// Copyright (c) 1995, 2009 by Embarcadero Technologies, Inc.
// All rights reserved

// (DO NOT EDIT: machine generated header) 'Strhlpr.pas' rev: 21.00

#ifndef StrhlprHPP
#define StrhlprHPP

// #pragma delphiheader begin
#pragma warning(push, 1)
// #pragma option push
// #pragma option -w-      // All warnings off
// #pragma option -Vx      // Zero-length empty class member functions
#pragma pack(push,8)
#include <System.hpp>	// Pascal unit
// #include <Sysinit.hpp>	// Pascal unit

//-- user supplied -----------------------------------------------------------
#define INEFFICIENT_COPY_OF_CONST_DELPHIRETURN_TYPES

namespace Strhlpr
{
//-- type declarations -------------------------------------------------------
//-- var, const, procedure ---------------------------------------------------
extern void __fastcall AnsiFromWide(System::RawByteString &dst, System::WideString &src);
extern void __fastcall AnsiFromUnicode(System::RawByteString &dst, System::UnicodeString &src);
extern void __fastcall AnsiFromPChar(System::RawByteString &dst, char * src);
extern void __fastcall AnsiFromPWideChar(System::RawByteString &dst, System::WideChar * src);
extern void __fastcall AnsiAppend(System::RawByteString &dst, System::RawByteString &src);
extern System::RawByteString __fastcall AnsiCat(System::RawByteString &x, System::RawByteString &y);
extern void __fastcall AnsiDelete(System::RawByteString &dst, int index, int count);
extern void __fastcall AnsiSetLength(System::RawByteString &dst, int len);
extern int __fastcall AnsiPos(System::RawByteString &src, System::RawByteString &sub);
extern System::RawByteString __fastcall AnsiCopy(System::RawByteString &src, int index, int count);
extern void __fastcall AnsiInsert(System::RawByteString &dst, System::RawByteString &src, int index);
extern bool __fastcall AnsiEqual(System::RawByteString &x, System::RawByteString &y);
extern void __fastcall AnsiAssign(System::RawByteString &dst, System::RawByteString &src);
extern void __fastcall AnsiFree(System::RawByteString &s);
extern void __fastcall WideAssign(System::WideString &dst, System::WideString &src);
extern void __fastcall WideFree(System::WideString &s);
extern void __fastcall WideFromAnsi(System::WideString &dst, System::RawByteString &src);
extern void __fastcall WideFromUnicode(System::WideString &dst, System::UnicodeString &src);
extern void __fastcall WideFromPChar(System::WideString &dst, char * src);
extern bool __fastcall WideEqual(System::WideString &x, System::WideString &y);
extern bool __fastcall WideLess(System::WideString &x, System::WideString &y);
extern bool __fastcall WideGreater(System::WideString &x, System::WideString &y);
extern System::WideString __fastcall WideCat(System::WideString &x, System::WideString &y);
extern int __fastcall WideLength(System::WideString &src);
extern int __fastcall WidePos(System::WideString &src, System::WideString &sub);
extern void __fastcall WideSetLength(System::WideString &dst, int len);
extern void __fastcall WideDelete(System::WideString &dst, int index, int count);
extern void __fastcall WideInsert(System::WideString &dst, System::WideString &src, int index);
extern System::WideString __fastcall WideCopy(System::WideString &src, int index, int count);
extern void __fastcall WideAppend(System::WideString &dst, System::WideString &src);
extern void __fastcall UnicodeFromWide(System::UnicodeString &dst, System::WideString &src);
extern void __fastcall UnicodeFromPChar(System::UnicodeString &dst, char * src);
extern void __fastcall UnicodeFromPWideChar(System::UnicodeString &dst, System::WideChar * src);
extern void __fastcall UnicodeFromAnsi(System::UnicodeString &dst, System::RawByteString &src);
extern void __fastcall UnicodeAppend(System::UnicodeString &dst, System::UnicodeString &src);
extern System::UnicodeString __fastcall UnicodeCat(System::UnicodeString &x, System::UnicodeString &y);
extern void __fastcall UnicodeDelete(System::UnicodeString &dst, int index, int count);
extern void __fastcall UnicodeSetLength(System::UnicodeString &dst, int len);
extern int __fastcall UnicodePos(System::UnicodeString &src, System::UnicodeString &sub);
extern System::UnicodeString __fastcall UnicodeCopy(System::UnicodeString &src, int index, int count);
extern void __fastcall UnicodeInsert(System::UnicodeString &dst, System::UnicodeString &src, int index);
extern void __fastcall UnicodeAssign(System::UnicodeString &dst, System::UnicodeString &src);
extern void __fastcall UnicodeFree(System::UnicodeString &s);
extern bool __fastcall UnicodeEqual(System::UnicodeString &x, System::UnicodeString &y);
extern bool __fastcall UnicodeLess(System::UnicodeString &x, System::UnicodeString &y);
extern bool __fastcall UnicodeGreater(System::UnicodeString &x, System::UnicodeString &y);

}	/* namespace Strhlpr */
// using namespace Strhlpr;
// #pragma pack(pop)
// #pragma option pop
#pragma warning(pop)

// #pragma delphiheader end.
//-- end unit ----------------------------------------------------------------
#endif	// StrhlprHPP
