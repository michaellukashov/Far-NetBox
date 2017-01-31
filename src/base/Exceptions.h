#pragma once

#include <Classes.hpp>
#include <Sysutils.hpp>
#include <SysInit.hpp>
#include <System.hpp>

bool ShouldDisplayException(Exception * E);
bool ExceptionMessage(const Exception * E, UnicodeString & Message);
bool ExceptionMessageFormatted(const Exception * E, UnicodeString & Message);
bool ExceptionFullMessage(Exception * E, UnicodeString & Message);
UnicodeString SysErrorMessageForError(int LastError);
UnicodeString LastSysErrorMessage();
TStrings * ExceptionToMoreMessages(Exception * E);
bool IsInternalException(const Exception * E);

enum TOnceDoneOperation
{
  odoIdle,
  odoDisconnect,
  odoSuspend,
  odoShutDown,
};

class ExtException : public Exception
{
public:
  TObjectClassId GetKind() const { return OBJECT_CLASS_ExtException; }
  static bool classof(const TObject * Obj)
  {
    TObjectClassId Kind = Obj->GetKind();
    return
      Kind == OBJECT_CLASS_ExtException ||
      Kind == OBJECT_CLASS_EFatal ||
      Kind == OBJECT_CLASS_ESshTerminate ||
      Kind == OBJECT_CLASS_ESsh ||
      Kind == OBJECT_CLASS_ETerminal ||
      Kind == OBJECT_CLASS_ECommand ||
      Kind == OBJECT_CLASS_EScp ||
      Kind == OBJECT_CLASS_ESkipFile ||
      Kind == OBJECT_CLASS_EFileSkipped;
  }
public:
  explicit ExtException(Exception * E);
  explicit ExtException(const Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"");
  // explicit ExtException(const ExtException * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"");
  explicit ExtException(Exception * E, int Ident);
  // "copy the exception", just append message to the end
  explicit ExtException(const UnicodeString & Msg, const Exception * E, const UnicodeString & HelpKeyword = L"");
  explicit ExtException(const UnicodeString & Msg, const UnicodeString & MoreMessages, const UnicodeString & HelpKeyword = L"");
  explicit ExtException(const UnicodeString & Msg, TStrings * MoreMessages, bool Own, const UnicodeString & HelpKeyword = L"");
  virtual ~ExtException(void) noexcept;
  TStrings * GetMoreMessages() const { return FMoreMessages; }
  const UnicodeString & GetHelpKeyword() const { return FHelpKeyword; }

  explicit inline ExtException(const UnicodeString & Msg) : Exception(Msg), FMoreMessages(nullptr) {}
  explicit inline ExtException(int Ident) : Exception(Ident), FMoreMessages(nullptr) {}
  explicit inline ExtException(const UnicodeString & Msg, int AHelpContext) : Exception(Msg, AHelpContext), FMoreMessages(nullptr) {}

  ExtException(const ExtException & E) : Exception(L""), FMoreMessages(nullptr), FHelpKeyword(E.FHelpKeyword)
  {
    AddMoreMessages(&E);
  }
  ExtException & operator = (const ExtException & rhs)
  {
    FHelpKeyword = rhs.FHelpKeyword;
    Message = rhs.Message;
    AddMoreMessages(&rhs);
    return *this;
  }

  static ExtException * CloneFrom(const Exception* E);

  virtual ExtException * Clone() const;

protected:
  void AddMoreMessages(const Exception * E);

private:
  TStrings * FMoreMessages;
  UnicodeString FHelpKeyword;
};

#define DERIVE_EXT_EXCEPTION(NAME, BASE) \
  class NAME : public BASE \
  { \
  public: \
    TObjectClassId GetKind() const { return OBJECT_CLASS_##NAME; } \
  public: \
    explicit inline NAME(const Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"") : BASE(E, Msg, HelpKeyword) {} \
    virtual inline ~NAME(void) noexcept {} \
    explicit inline NAME(const UnicodeString & Msg, int AHelpContext) : BASE(Msg, AHelpContext) {} \
    virtual ExtException * Clone() const { return new NAME(this, L""); } \
  };

DERIVE_EXT_EXCEPTION(ESsh, ExtException)
DERIVE_EXT_EXCEPTION(ETerminal, ExtException)
DERIVE_EXT_EXCEPTION(ECommand, ExtException)
DERIVE_EXT_EXCEPTION(EScp, ExtException) // SCP protocol fatal error (non-fatal in application context)
DERIVE_EXT_EXCEPTION(ESkipFile, ExtException)
DERIVE_EXT_EXCEPTION(EFileSkipped, ESkipFile)

class EOSExtException : public ExtException
{
public:
  TObjectClassId GetKind() const { return OBJECT_CLASS_EOSExtException; }
  static bool classof(const TObject * Obj)
  {
    TObjectClassId Kind = Obj->GetKind();
    return
      Kind == OBJECT_CLASS_EOSExtException ||
      Kind == OBJECT_CLASS_ECRTExtException;
  }
public:
  explicit EOSExtException();
  explicit EOSExtException(const UnicodeString & Msg);
  explicit EOSExtException(const UnicodeString & Msg, int LastError);
};

class ECRTExtException : public EOSExtException
{
public:
  TObjectClassId GetKind() const { return OBJECT_CLASS_ECRTExtException; }
  static bool classof(const TObject * Obj)
  {
    TObjectClassId Kind = Obj->GetKind();
    return
      Kind == OBJECT_CLASS_ECRTExtException;
  }
public:
  ECRTExtException();
  explicit ECRTExtException(const UnicodeString & Msg);
};

class EFatal : public ExtException
{
public:
  TObjectClassId GetKind() const { return OBJECT_CLASS_EFatal; }
  static inline bool classof(const TObject * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_EFatal ||
      Obj->GetKind() == OBJECT_CLASS_ESshFatal ||
      Obj->GetKind() == OBJECT_CLASS_ESshTerminate;
  }
public:
  // fatal errors are always copied, new message is only appended
  explicit EFatal(const Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"");

  bool GetReopenQueried() const { return FReopenQueried; }
  void SetReopenQueried(bool Value) { FReopenQueried = Value; }

  virtual ExtException * Clone() const;

private:
  bool FReopenQueried;
};

#define DERIVE_FATAL_EXCEPTION(NAME, BASE) \
  class NAME : public BASE \
  { \
  public: \
    TObjectClassId GetKind() const { return OBJECT_CLASS_##NAME; } \
  public: \
    explicit inline NAME(const Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"") : BASE(E, Msg, HelpKeyword) {} \
    virtual ExtException * Clone() const { return new NAME(this, L""); } \
  };

DERIVE_FATAL_EXCEPTION(ESshFatal, EFatal)

// exception that closes application, but displays info message (not error message)
// = close on completion
class ESshTerminate : public EFatal
{
public:
  TObjectClassId GetKind() const { return OBJECT_CLASS_ESshTerminate; }
  static inline bool classof(const TObject * Obj)
  {
   return
     Obj->GetKind() == OBJECT_CLASS_ESshTerminate;
  }
public:
  explicit inline ESshTerminate(const Exception * E, const UnicodeString & Msg, TOnceDoneOperation AOperation) :
    EFatal(E, Msg),
    Operation(AOperation)
  {
  }

  virtual ExtException * Clone() const;

  TOnceDoneOperation Operation;
};

class ECallbackGuardAbort : public EAbort
{
public:
  TObjectClassId GetKind() const { return OBJECT_CLASS_ECallbackGuardAbort; }
  static inline bool classof(const TObject * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_ECallbackGuardAbort;
  }
public:
  ECallbackGuardAbort();
};

Exception * CloneException(Exception * Exception);
void RethrowException(Exception * E);
UnicodeString GetExceptionHelpKeyword(const Exception* E);
UnicodeString MergeHelpKeyword(const UnicodeString & PrimaryHelpKeyword, const UnicodeString & SecondaryHelpKeyword);
bool IsInternalErrorHelpKeyword(const UnicodeString & HelpKeyword);

