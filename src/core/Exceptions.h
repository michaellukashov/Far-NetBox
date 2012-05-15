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

  explicit /* __fastcall */ ExtException(UnicodeString Msg) : Sysutils::Exception(Msg), FMoreMessages(NULL) {}
  explicit /* __fastcall */ ExtException(int Ident) : Sysutils::Exception(Ident), FMoreMessages(NULL) {}
  explicit /* __fastcall */ ExtException(UnicodeString Msg, int AHelpContext) : Sysutils::Exception(Msg, AHelpContext), FMoreMessages(NULL) {}

  /* __fastcall */ ExtException(ExtException & E) : Sysutils::Exception(E.GetMessage()), FMoreMessages(NULL)
  { AddMoreMessages(&E); }
  ExtException & operator =(const ExtException &rhs)
  { SetMessage(rhs.GetMessage()); AddMoreMessages(&rhs); }

  virtual ExtException * __fastcall Clone();

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
    explicit inline /* __fastcall */ NAME(Exception* E, UnicodeString Msg) : BASE(E, Msg) {} \
    virtual inline /* __fastcall */ ~NAME(void) { } \
    explicit inline  /* __fastcall */ NAME(const UnicodeString Msg, int AHelpContext) : BASE(Msg, AHelpContext) { } \
    virtual ExtException * __fastcall Clone() { return new NAME(this, L""); } \
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

  virtual ExtException * __fastcall Clone();

private:
  bool FReopenQueried;
};
//---------------------------------------------------------------------------
#define DERIVE_FATAL_EXCEPTION(NAME, BASE) \
  class NAME : public BASE \
  { \
  public: \
    explicit inline /* __fastcall */ NAME(Exception* E, UnicodeString Msg) : BASE(E, Msg) {} \
    virtual ExtException * __fastcall Clone() { return new NAME(this, L""); } \
  };
//---------------------------------------------------------------------------
DERIVE_FATAL_EXCEPTION(ESshFatal, EFatal);
//---------------------------------------------------------------------------
// exception that closes application, but displayes info message (not error message)
// = close on completion
class ESshTerminate : public EFatal
{
public:
  explicit inline /* __fastcall */ ESshTerminate(Exception* E, UnicodeString Msg, TOnceDoneOperation AOperation) :
    EFatal(E, Msg),
    Operation(AOperation)
  {}

  virtual ExtException * __fastcall Clone();

  TOnceDoneOperation Operation;
};
//---------------------------------------------------------------------------
class ECallbackGuardAbort : public EAbort
{
public:
  /* __fastcall */ ECallbackGuardAbort();
};
//---------------------------------------------------------------------------
Exception * __fastcall CloneException(Exception * Exception);
void __fastcall RethrowException(Exception * E);
//---------------------------------------------------------------------------
#endif  // Exceptions
