//---------------------------------------------------------------------------
#ifndef ExceptionsH
#define ExceptionsH

#ifndef _MSC_VER
#include <Classes.hpp>
#include <SysUtils.hpp>
#include <SysInit.hpp>
#include <System.hpp>
#else
#include "stdafx.h"
#include "Classes.h"
#include "SysUtils.h"
#endif

//---------------------------------------------------------------------------
bool __fastcall ExceptionMessage(const Exception * E, UnicodeString & Message);
UnicodeString __fastcall LastSysErrorMessage();
TStrings * ExceptionToMoreMessages(Exception * E);
//---------------------------------------------------------------------------
enum TOnceDoneOperation { odoIdle, odoDisconnect, odoShutDown };
//---------------------------------------------------------------------------
class ExtException : public Sysutils::Exception
{
public:
  explicit /* __fastcall */ ExtException(Exception* E);
  explicit /* __fastcall */ ExtException(Exception* E, UnicodeString Msg);
  // "copy the exception", just append message to the end
  explicit /* __fastcall */ ExtException(UnicodeString Msg, Exception* E);
  explicit /* __fastcall */ ExtException(UnicodeString Msg, UnicodeString MoreMessages, UnicodeString HelpKeyword = L"");
  explicit /* __fastcall */ ExtException(UnicodeString Msg, TStrings* MoreMessages, bool Own, UnicodeString HelpKeyword = L"");
  virtual /* __fastcall */ ~ExtException(void);
#ifndef _MSC_VER
  __property TStrings* MoreMessages = {read=FMoreMessages};
  __property UnicodeString HelpKeyword = {read=FHelpKeyword};
#else
  TStrings * /* __fastcall */ GetMoreMessages() const { return FMoreMessages; }
  UnicodeString /* __fastcall */ GetHelpKeyword() const { return FHelpKeyword; }
#endif

  // inline __fastcall ExtException(const UnicodeString Msg, const TVarRec * Args, const int Args_Size) : Sysutils::Exception(Msg, Args, Args_Size) { }
  explicit /* __fastcall */ ExtException(const UnicodeString Msg) : Sysutils::Exception(Msg), FMoreMessages(NULL) {}
  // inline __fastcall ExtException(int Ident, const TVarRec * Args, const int Args_Size)/* overload */ : Sysutils::Exception(Ident, Args, Args_Size) { }
  explicit /* __fastcall */ ExtException(int Ident) : Sysutils::Exception(Ident), FMoreMessages(NULL) {}
  explicit /* __fastcall */ ExtException(const UnicodeString Msg, int AHelpContext) : Sysutils::Exception(Msg, AHelpContext), FMoreMessages(NULL) {}
  // inline __fastcall ExtException(const UnicodeString Msg, const TVarRec * Args, const int Args_Size, int AHelpContext) : Sysutils::Exception(Msg, Args, Args_Size, AHelpContext) { }
  inline /* __fastcall */ ExtException(int Ident, int AHelpContext)/* overload */ : Sysutils::Exception(Ident, AHelpContext), FMoreMessages(NULL) {}

  /* __fastcall */ ExtException(ExtException & E) : Sysutils::Exception(E.GetMessage()), FMoreMessages(NULL)
  { AddMoreMessages(&E); }
  ExtException & operator =(const ExtException &rhs)
  { SetMessage(rhs.GetMessage()); AddMoreMessages(&rhs); }

protected:
  void __fastcall AddMoreMessages(const Exception* E);

private:
  Classes::TStrings* FMoreMessages;
  UnicodeString FHelpKeyword;
};
//---------------------------------------------------------------------------
#define DERIVE_EXT_EXCEPTION(NAME, BASE) \
  class NAME : public BASE \
  { \
  public: \
    explicit /* __fastcall */ NAME(Exception* E, UnicodeString Msg) : BASE(E, Msg) {} \
    explicit /* __fastcall */ NAME(Exception* E, int Ident) : BASE(E, Ident) {} \
    virtual /* __fastcall */ ~NAME(void) { } \
    /* inline __fastcall NAME(const UnicodeString Msg, const TVarRec * Args, const int Args_Size) : BASE(Msg, Args, Args_Size) { } */ \
    /* inline __fastcall NAME(int Ident, const TVarRec * Args, const int Args_Size) : BASE(Ident, Args, Args_Size) { } */ \
    explicit /* __fastcall */ NAME(const UnicodeString Msg, int AHelpContext) : BASE(Msg, AHelpContext) { } \
    /* inline __fastcall NAME(const UnicodeString Msg, const TVarRec * Args, const int Args_Size, int AHelpContext) : BASE(Msg, Args, Args_Size, AHelpContext) { } */ \
    explicit /* __fastcall */ NAME(int Ident, int AHelpContext) : BASE(Ident, AHelpContext) { } \
    /* inline __fastcall NAME(PResStringRec ResStringRec, const TVarRec * Args, const int Args_Size, int AHelpContext) : BASE(ResStringRec, Args, Args_Size, AHelpContext) { } */ \
  };
//---------------------------------------------------------------------------
DERIVE_EXT_EXCEPTION(ESsh, ExtException);
DERIVE_EXT_EXCEPTION(ETerminal, ExtException);
DERIVE_EXT_EXCEPTION(ECommand, ExtException);
DERIVE_EXT_EXCEPTION(EScp, ExtException); // SCP protocol fatal error (non-fatal in application context)
DERIVE_EXT_EXCEPTION(EScpSkipFile, ExtException);
DERIVE_EXT_EXCEPTION(EScpFileSkipped, EScpSkipFile);
//---------------------------------------------------------------------------
class EOSExtException : public ExtException
{
public:
  explicit /* __fastcall */ EOSExtException();
  explicit /* __fastcall */ EOSExtException(UnicodeString Msg);
};
//---------------------------------------------------------------------------
class EFatal : public ExtException
{
public:
  // fatal errors are always copied, new message is only appended
  explicit /* __fastcall */ EFatal(Exception* E, UnicodeString Msg);

#ifndef _MSC_VER
  __property bool ReopenQueried = { read = FReopenQueried, write = FReopenQueried };
#else
  bool /* __fastcall */ GetReopenQueried() { return FReopenQueried; }
  void /* __fastcall */ SetReopenQueried(bool value) { FReopenQueried = value; }
#endif

private:
  bool FReopenQueried;
};
//---------------------------------------------------------------------------
#define DERIVE_FATAL_EXCEPTION(NAME, BASE) \
  class NAME : public BASE \
  { \
  public: \
    explicit /* __fastcall */ NAME(Exception* E, UnicodeString Msg) : BASE(E, Msg) {} \
  };
//---------------------------------------------------------------------------
DERIVE_FATAL_EXCEPTION(ESshFatal, EFatal);
//---------------------------------------------------------------------------
// exception that closes application, but displayes info message (not error message)
// = close on completionclass ESshTerminate : public EFatal
class ESshTerminate : public EFatal
{
public:
  explicit /* __fastcall */ ESshTerminate(Exception* E, UnicodeString Msg, TOnceDoneOperation AOperation) :
    EFatal(E, Msg),
    Operation(AOperation)
  {}

  TOnceDoneOperation Operation;
};
//---------------------------------------------------------------------------
#endif  // Exceptions
