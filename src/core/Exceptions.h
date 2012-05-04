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
#endif

class Exception;
//---------------------------------------------------------------------------
bool __fastcall ExceptionMessage(Exception * E, UnicodeString & Message);
UnicodeString __fastcall LastSysErrorMessage();
TStrings * ExceptionToMoreMessages(Exception * E);
//---------------------------------------------------------------------------
enum TOnceDoneOperation { odoIdle, odoDisconnect, odoShutDown };
//---------------------------------------------------------------------------
class Exception : public std::exception, public TObject
{
  typedef std::exception parent;
public:
  explicit /* __fastcall */ Exception(const UnicodeString Msg);
  explicit /* __fastcall */ Exception(const Exception & E);
  explicit /* __fastcall */ Exception(const std::exception * E);
  template<typename T>
  bool InheritsFrom() const { return dynamic_cast<const T *>(this) != NULL; }

  UnicodeString GetHelpKeyword() const { return FHelpKeyword; }
  const UnicodeString GetMessage() const { return FMessage; }
  void SetMessage(const UnicodeString value) { FMessage = value; }
protected:
  UnicodeString FMessage;
  UnicodeString FHelpKeyword;
};
//---------------------------------------------------------------------------
class ExtException : public Exception
{
  typedef Exception parent;
public:
  explicit /* __fastcall */ ExtException(const std::exception * E);
  explicit /* __fastcall */ ExtException(Exception* E);
  explicit /* __fastcall */ ExtException(Exception* E, UnicodeString Msg);
  explicit /* __fastcall */ ExtException(UnicodeString Msg);
  // "copy the exception", just append message to the end
  explicit /* __fastcall */ ExtException(UnicodeString Msg, Exception * E);
  // explicit /* __fastcall */ ExtException(UnicodeString Msg, UnicodeString MoreMessages, UnicodeString HelpKeyword = L"");
  explicit /* __fastcall */ ExtException(UnicodeString Msg, TStrings * MoreMessages, bool Own);
  explicit /* __fastcall */ ExtException(ExtException &) throw();
  ExtException  & /* __fastcall */ operator =(const ExtException &) throw();
  virtual /* __fastcall */ ~ExtException(void) throw();
#ifndef _MSC_VER
  __property TStrings* MoreMessages = {read=FMoreMessages};
  __property UnicodeString HelpKeyword = {read=FHelpKeyword};
#endif

  TStrings * /* __fastcall */ GetMoreMessages() const { return FMoreMessages; }
  UnicodeString /* __fastcall */ GetHelpKeyword() { return FHelpKeyword; }
protected:
  void __fastcall AddMoreMessages(const Exception* E);

private:
  TStrings* FMoreMessages;
  UnicodeString FHelpKeyword;
};
//---------------------------------------------------------------------------
#define DERIVE_EXT_EXCEPTION(NAME, BASE) \
  class NAME : public BASE \
  { \
    typedef BASE parent; \
  public: \
    explicit /* __fastcall */ NAME(UnicodeString Msg, Exception* E) : parent(Msg, E) {} \
    virtual /* __fastcall */ ~NAME(void) throw() {} \
  };

//---------------------------------------------------------------------------
DERIVE_EXT_EXCEPTION(ESsh, ExtException);
DERIVE_EXT_EXCEPTION(ETerminal, ExtException);
DERIVE_EXT_EXCEPTION(ECommand, ExtException);
DERIVE_EXT_EXCEPTION(EScp, ExtException); // SCP protocol fatal error (non-fatal in application context)
DERIVE_EXT_EXCEPTION(EScpSkipFile, ExtException);
DERIVE_EXT_EXCEPTION(EScpFileSkipped, EScpSkipFile);
//---------------------------------------------------------------------------
/*
class EOSExtException : public ExtException
{
  typedef ExtException parent;
public:
  __fastcall EOSExtException();
  __fastcall EOSExtException(UnicodeString Msg);
};
*/
//---------------------------------------------------------------------------
class EFatal : public ExtException
{
  typedef ExtException parent;
public:
  // fatal errors are always copied, new message is only appended
  explicit /* __fastcall */ EFatal(UnicodeString Msg, Exception * E);

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
    typedef BASE parent; \
  public: \
    explicit /* __fastcall */ NAME(UnicodeString Msg, Exception* E) : parent(Msg, E) {} \
  };
//---------------------------------------------------------------------------
DERIVE_FATAL_EXCEPTION(ESshFatal, EFatal);
//---------------------------------------------------------------------------
// exception that closes application, but displayes info message (not error message)
// = close on completionclass ESshTerminate : public EFatal
class ESshTerminate : public EFatal
{
  typedef EFatal parent;
public:
  explicit /* __fastcall */ ESshTerminate(UnicodeString Msg, Exception * E, TOnceDoneOperation AOperation) :
    parent(Msg, E),
    Operation(AOperation)
  {}

  TOnceDoneOperation Operation;
};
//---------------------------------------------------------------------------
#endif  // Exceptions
