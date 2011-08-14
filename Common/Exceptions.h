//---------------------------------------------------------------------------
#ifndef ExceptionsH
#define ExceptionsH

#include <Classes.hpp>
#include <SysUtils.hpp>
#include <SysInit.hpp>
#include <System.hpp>
//---------------------------------------------------------------------------
bool ExceptionMessage(exception * E, wstring & Message);
wstring LastSysErrorMessage();
TStrings * ExceptionToMoreMessages(exception * E);
//---------------------------------------------------------------------------
enum TOnceDoneOperation { odoIdle, odoDisconnect, odoShutDown };
//---------------------------------------------------------------------------
class ExtException : public Sysutils::exception
{
public:
  ExtException(exception* E);
  ExtException(exception* E, wstring Msg);
  // "copy the exception", just append message to the end
  ExtException(wstring Msg, exception* E);
  ExtException(wstring Msg, wstring MoreMessages, wstring HelpKeyword = "");
  ExtException(wstring Msg, TStrings* MoreMessages, bool Own);
  virtual ~ExtException(void);
  __property TStrings* MoreMessages = {read=FMoreMessages};
  __property wstring HelpKeyword = {read=FHelpKeyword};

  inline ExtException(const wstring Msg, const TVarRec * Args, const int Args_Size) : Sysutils::exception(Msg, Args, Args_Size) { }
  inline ExtException(int Ident, const TVarRec * Args, const int Args_Size)/* overload */ : Sysutils::exception(Ident, Args, Args_Size) { }
  inline ExtException(const wstring Msg, int AHelpContext) : Sysutils::exception(Msg, AHelpContext) { }
  inline ExtException(const wstring Msg, const TVarRec * Args, const int Args_Size, int AHelpContext) : Sysutils::exception(Msg, Args, Args_Size, AHelpContext) { }
  inline ExtException(int Ident, int AHelpContext)/* overload */ : exception(Ident, AHelpContext) { }
  inline ExtException(PResStringRec ResStringRec, const TVarRec * Args, const int Args_Size, int AHelpContext)/* overload */ : Sysutils::exception(ResStringRec, Args, Args_Size, AHelpContext) { }

protected:
  void AddMoreMessages(exception* E);

private:
  Classes::TStrings* FMoreMessages;
  wstring FHelpKeyword;
};
//---------------------------------------------------------------------------
#define DERIVE_EXT_EXCEPTION(NAME, BASE) \
  class NAME : public BASE \
  { \
  public: \
    inline NAME(exception* E, wstring Msg) : BASE(E, Msg) { } \
    inline NAME(exception* E, int Ident) : BASE(E, Ident) { } \
    inline virtual ~NAME(void) { } \
    inline NAME(const wstring Msg, const TVarRec * Args, const int Args_Size) : BASE(Msg, Args, Args_Size) { } \
    inline NAME(int Ident, const TVarRec * Args, const int Args_Size) : BASE(Ident, Args, Args_Size) { } \
    inline NAME(const wstring Msg, int AHelpContext) : BASE(Msg, AHelpContext) { } \
    inline NAME(const wstring Msg, const TVarRec * Args, const int Args_Size, int AHelpContext) : BASE(Msg, Args, Args_Size, AHelpContext) { } \
    inline NAME(int Ident, int AHelpContext) : BASE(Ident, AHelpContext) { } \
    inline NAME(PResStringRec ResStringRec, const TVarRec * Args, const int Args_Size, int AHelpContext) : BASE(ResStringRec, Args, Args_Size, AHelpContext) { } \
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
  EOSExtException();
  EOSExtException(wstring Msg);
};
//---------------------------------------------------------------------------
class EFatal : public ExtException
{
public:
  // fatal errors are always copied, new message is only appended
  inline EFatal(exception* E, wstring Msg) : ExtException(Msg, E) { }
};
//---------------------------------------------------------------------------
#define DERIVE_FATAL_EXCEPTION(NAME, BASE) \
  class NAME : public BASE \
  { \
  public: \
    inline NAME(exception* E, wstring Msg) : BASE(E, Msg) { } \
  };
//---------------------------------------------------------------------------
DERIVE_FATAL_EXCEPTION(ESshFatal, EFatal);
//---------------------------------------------------------------------------
// exception that closes application, but displayes info message (not error message)
// = close on completionclass ESshTerminate : public EFatal
class ESshTerminate : public EFatal
{
public:
  inline ESshTerminate(exception* E, wstring Msg, TOnceDoneOperation AOperation) :
    EFatal(E, Msg),
    Operation(AOperation)
  { }

  TOnceDoneOperation Operation;
};
//---------------------------------------------------------------------------
#endif  // Exceptions
