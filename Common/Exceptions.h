//---------------------------------------------------------------------------
#pragma once

#include "stdafx.h"
#include "Classes.h"
#include "FarUtil.h"
// #include <SysUtils.hpp>
// #include <SysInit.hpp>
// #include <System.hpp>

//---------------------------------------------------------------------------
bool ExceptionMessage(exception * E, std::wstring & Message);
std::wstring LastSysErrorMessage();
TStrings * ExceptionToMoreMessages(exception * E);
//---------------------------------------------------------------------------
enum TOnceDoneOperation { odoIdle, odoDisconnect, odoShutDown };
//---------------------------------------------------------------------------
class ExtException : public exception
{
public:
  ExtException(exception* E);
  ExtException(exception* E, std::wstring Msg);
  // "copy the exception", just append message to the end
  ExtException(std::wstring Msg);
  ExtException(std::wstring Msg, exception* E);
  ExtException(std::wstring Msg, std::wstring MoreMessages, std::wstring HelpKeyword = L"");
  ExtException(std::wstring Msg, TStrings* MoreMessages, bool Own);
  virtual ~ExtException(void);
  TStrings* GetMoreMessages() { return FMoreMessages; }
  std::wstring GetHelpKeyword() { return FHelpKeyword; }

  // inline ExtException(const std::wstring Msg, const TVarRec * Args, const int Args_Size) : Sysutils::exception(Msg, Args, Args_Size) { }
  // inline ExtException(int Ident, const TVarRec * Args, const int Args_Size)/* overload */ : Sysutils::exception(Ident, Args, Args_Size) { }
  inline ExtException(const std::wstring Msg, int AHelpContext) : exception(::W2MB(Msg.c_str()).c_str(), AHelpContext) { }
  // inline ExtException(const std::wstring Msg, const TVarRec * Args, const int Args_Size, int AHelpContext) : Sysutils::exception(Msg, Args, Args_Size, AHelpContext) { }
  // inline ExtException(int Ident, int AHelpContext)/* overload */ : exception(Ident, AHelpContext) { }
  // inline ExtException(PResStringRec ResStringRec, const TVarRec * Args, const int Args_Size, int AHelpContext)/* overload */ : Sysutils::exception(ResStringRec, Args, Args_Size, AHelpContext) { }
  const std::wstring GetMessage() const { return FMessage; }
  void SetMessage(const std::wstring value) { FMessage = value; }
protected:
  void AddMoreMessages(exception* E);

private:
  TStrings *FMoreMessages;
  std::wstring FHelpKeyword;
  std::wstring FMessage;
};
//---------------------------------------------------------------------------
#define DERIVE_EXT_EXCEPTION(NAME, BASE) \
  class NAME : public BASE \
  { \
  public: \
    inline NAME(exception* E, std::wstring Msg) : BASE(E, Msg) { } \
    inline virtual ~NAME(void) { } \
    inline NAME(const std::wstring Msg, int AHelpContext) : BASE(Msg, AHelpContext) { } \
  };

    // inline NAME(exception* E, int Ident) : BASE(E, Ident) { } \
    // inline NAME(const std::wstring Msg, const TVarRec * Args, const int Args_Size) : BASE(Msg, Args, Args_Size) { } \
    // inline NAME(int Ident, const TVarRec * Args, const int Args_Size) : BASE(Ident, Args, Args_Size) { } \
    // inline NAME(const std::wstring Msg, const TVarRec * Args, const int Args_Size, int AHelpContext) : BASE(Msg, Args, Args_Size, AHelpContext) { } \
    // inline NAME(PResStringRec ResStringRec, const TVarRec * Args, const int Args_Size, int AHelpContext) : BASE(ResStringRec, Args, Args_Size, AHelpContext) { } \
    // inline NAME(int Ident, int AHelpContext) : BASE(Ident, AHelpContext) { } \

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
  EOSExtException(std::wstring Msg);
};
//---------------------------------------------------------------------------
class EFatal : public ExtException
{
public:
  // fatal errors are always copied, new message is only appended
  inline EFatal(exception* E, std::wstring Msg) : ExtException(Msg, E) { }
};
//---------------------------------------------------------------------------
#define DERIVE_FATAL_EXCEPTION(NAME, BASE) \
  class NAME : public BASE \
  { \
  public: \
    inline NAME(exception* E, std::wstring Msg) : BASE(E, Msg) { } \
  };
//---------------------------------------------------------------------------
DERIVE_FATAL_EXCEPTION(ESshFatal, EFatal);
//---------------------------------------------------------------------------
// exception that closes application, but displayes info message (not error message)
// = close on completionclass ESshTerminate : public EFatal
class ESshTerminate : public EFatal
{
public:
  inline ESshTerminate(exception* E, std::wstring Msg, TOnceDoneOperation AOperation) :
    EFatal(E, Msg),
    Operation(AOperation)
  { }

  TOnceDoneOperation Operation;
};
//---------------------------------------------------------------------------
