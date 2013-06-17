//---------------------------------------------------------------------------
#ifndef ExceptionsH
#define ExceptionsH

#include <Classes.hpp>
#include <SysUtils.hpp>
#include <SysInit.hpp>
#include <System.hpp>
//---------------------------------------------------------------------------
bool ExceptionMessage(const Exception * E, UnicodeString & Message);
UnicodeString LastSysErrorMessage();
TStrings * ExceptionToMoreMessages(Exception * E);
//---------------------------------------------------------------------------
enum TOnceDoneOperation { odoIdle, odoDisconnect, odoShutDown };
//---------------------------------------------------------------------------
class ExtException : public Sysutils::Exception
{
public:
  explicit ExtException(Exception * E);
  explicit ExtException(Exception * E, const UnicodeString & Msg);
  explicit ExtException(ExtException * E, const UnicodeString & Msg);
  explicit ExtException(Exception * E, int Ident);
  // "copy the exception", just append message to the end
  explicit ExtException(const UnicodeString & Msg, Exception * E);
  explicit ExtException(const UnicodeString & Msg, const UnicodeString & MoreMessages, const UnicodeString & HelpKeyword = UnicodeString());
  explicit ExtException(const UnicodeString & Msg, TStrings * MoreMessages, bool Own, const UnicodeString & HelpKeyword = UnicodeString());
  virtual ~ExtException(void) throw();
  TStrings * GetMoreMessages() const { return FMoreMessages; }
  UnicodeString GetHelpKeyword() const { return FHelpKeyword; }

  explicit ExtException(const UnicodeString & Msg) : Sysutils::Exception(Msg), FMoreMessages(nullptr) {}
  explicit ExtException(int Ident) : Sysutils::Exception(Ident), FMoreMessages(nullptr) {}
  explicit ExtException(const UnicodeString & Msg, int AHelpContext) : Sysutils::Exception(Msg, AHelpContext), FMoreMessages(nullptr) {}

  ExtException(const ExtException & E) : Sysutils::Exception(L""), FMoreMessages(nullptr)
  { AddMoreMessages(&E); }
  ExtException & operator =(const ExtException &rhs)
  { Message = rhs.Message; AddMoreMessages(&rhs); return *this; }

  virtual ExtException * Clone();

protected:
  void AddMoreMessages(const Exception * E);

private:
  Classes::TStrings* FMoreMessages;
  UnicodeString FHelpKeyword;
};
//---------------------------------------------------------------------------
#define DERIVE_EXT_EXCEPTION(NAME, BASE) \
  class NAME : public BASE \
  { \
  public: \
    explicit inline NAME(Exception* E, const UnicodeString & Msg) : BASE(E, Msg) {} \
    virtual inline ~NAME(void) throw() {} \
    explicit inline  NAME(const UnicodeString & Msg, int AHelpContext) : BASE(Msg, AHelpContext) {} \
    virtual ExtException * Clone() { return new NAME(this, L""); } \
  };
//---------------------------------------------------------------------------
DERIVE_EXT_EXCEPTION(ESsh, ExtException)
DERIVE_EXT_EXCEPTION(ETerminal, ExtException)
DERIVE_EXT_EXCEPTION(ECommand, ExtException)
DERIVE_EXT_EXCEPTION(EScp, ExtException) // SCP protocol fatal error (non-fatal in application context)
DERIVE_EXT_EXCEPTION(EScpSkipFile, ExtException)
DERIVE_EXT_EXCEPTION(EScpFileSkipped, EScpSkipFile)
//---------------------------------------------------------------------------
class EOSExtException : public ExtException
{
public:
  explicit EOSExtException();
  explicit EOSExtException(const UnicodeString & Msg);
};
//---------------------------------------------------------------------------
class EFatal : public ExtException
{
public:
  // fatal errors are always copied, new message is only appended
  explicit EFatal(Exception* E, const UnicodeString & Msg);

  bool GetReopenQueried() const { return FReopenQueried; }
  void SetReopenQueried(bool Value) { FReopenQueried = Value; }

  virtual ExtException * Clone();

private:
  bool FReopenQueried;
};
//---------------------------------------------------------------------------
#define DERIVE_FATAL_EXCEPTION(NAME, BASE) \
  class NAME : public BASE \
  { \
  public: \
    explicit inline NAME(Exception* E, const UnicodeString & Msg) : BASE(E, Msg) {} \
    virtual ExtException * Clone() { return new NAME(this, L""); } \
  };
//---------------------------------------------------------------------------
DERIVE_FATAL_EXCEPTION(ESshFatal, EFatal)
//---------------------------------------------------------------------------
// exception that closes application, but displays info message (not error message)
// = close on completion
class ESshTerminate : public EFatal
{
public:
  explicit inline ESshTerminate(Exception* E, const UnicodeString & Msg, TOnceDoneOperation AOperation) :
    EFatal(E, Msg),
    Operation(AOperation)
  {}

  virtual ExtException * Clone();

  TOnceDoneOperation Operation;
};
//---------------------------------------------------------------------------
class ECallbackGuardAbort : public EAbort
{
public:
  ECallbackGuardAbort();
};
//---------------------------------------------------------------------------
Exception * CloneException(Exception * Exception);
void RethrowException(Exception * E);
//---------------------------------------------------------------------------
#endif  // Exceptions
