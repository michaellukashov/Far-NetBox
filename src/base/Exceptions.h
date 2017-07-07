
#pragma once

#include <Classes.hpp>
#include <Sysutils.hpp>
#include <SysInit.hpp>
#include <System.hpp>

bool ShouldDisplayException(Exception * E);
bool ExceptionMessage(const Exception * E, UnicodeString & Message);
bool ExceptionMessageFormatted(const Exception * E, UnicodeString & Message);
bool ExceptionFullMessage(Exception * E, UnicodeString & Message);
UnicodeString SysErrorMessageForError(intptr_t LastError);
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
  static bool classof(const Exception * Obj)
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
      Kind == OBJECT_CLASS_EFileSkipped ||
      Kind == OBJECT_CLASS_EStreamError;
  }
public:
  explicit ExtException(Exception * E);
  explicit ExtException(TObjectClassId Kind, Exception * E);
  explicit ExtException(const Exception * E, UnicodeString Msg, UnicodeString HelpKeyword = L"");
  explicit ExtException(TObjectClassId Kind, const Exception * E, UnicodeString Msg, UnicodeString HelpKeyword = L"");
  // explicit ExtException(const ExtException * E, UnicodeString Msg, UnicodeString HelpKeyword = L"");
  explicit ExtException(TObjectClassId Kind, Exception * E, intptr_t Ident);
  // "copy the exception", just append message to the end
  explicit ExtException(TObjectClassId Kind, UnicodeString Msg, const Exception * E, UnicodeString HelpKeyword = L"");
  explicit ExtException(UnicodeString Msg, UnicodeString MoreMessages, UnicodeString HelpKeyword = L"");
  explicit ExtException(TObjectClassId Kind, UnicodeString Msg, UnicodeString MoreMessages, UnicodeString HelpKeyword = L"");
  explicit ExtException(UnicodeString Msg, TStrings * MoreMessages, bool Own, UnicodeString HelpKeyword = L"");
  virtual ~ExtException(void);
#if 0
  __property TStrings* MoreMessages = {read=FMoreMessages};
  __property UnicodeString HelpKeyword = {read=FHelpKeyword};
#endif // #if 0
  TStrings * GetMoreMessages() const { return FMoreMessages; }
  UnicodeString GetHelpKeyword() const { return FHelpKeyword; }

  explicit inline ExtException(UnicodeString Msg) : Exception(OBJECT_CLASS_ExtException, Msg), FMoreMessages(nullptr) {}
  explicit inline ExtException(TObjectClassId Kind, UnicodeString Msg) : Exception(Kind, Msg), FMoreMessages(nullptr) {}
  explicit inline ExtException(TObjectClassId Kind, intptr_t Ident) : Exception(Kind, Ident), FMoreMessages(nullptr) {}
  explicit inline ExtException(TObjectClassId Kind, UnicodeString Msg, intptr_t AHelpContext) : Exception(Kind, Msg, AHelpContext), FMoreMessages(nullptr) {}

  ExtException(const ExtException & E) : Exception(OBJECT_CLASS_ExtException, L""), FMoreMessages(nullptr), FHelpKeyword(E.FHelpKeyword)
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

  static ExtException * CloneFrom(const Exception * E);

  virtual ExtException * Clone() const;
  virtual void Rethrow();

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
    static inline bool classof(const Exception * Obj) { return Obj->GetKind() == OBJECT_CLASS_##NAME; } \
  public: \
    explicit inline NAME(TObjectClassId Kind, const Exception * E, UnicodeString Msg, UnicodeString HelpKeyword = L"") : BASE(Kind, E, Msg, HelpKeyword) {} \
    explicit inline NAME(const Exception * E, UnicodeString Msg, UnicodeString HelpKeyword = L"") : BASE(OBJECT_CLASS_##NAME, E, Msg, HelpKeyword) {} \
    explicit inline NAME(TObjectClassId Kind, UnicodeString Msg, intptr_t AHelpContext) : BASE(Kind, Msg, AHelpContext) {} \
    explicit inline NAME(UnicodeString Msg, intptr_t AHelpContext) : BASE(OBJECT_CLASS_##NAME, Msg, AHelpContext) {} \
    virtual inline ~NAME(void) {} \
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
  static bool classof(const Exception * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_EOSExtException ||
      Obj->GetKind() == OBJECT_CLASS_ECRTExtException;
  }
public:
  explicit EOSExtException();
  explicit EOSExtException(UnicodeString Msg);
  explicit EOSExtException(UnicodeString Msg, intptr_t LastError);
  explicit EOSExtException(TObjectClassId Kind, UnicodeString Msg, intptr_t LastError);
};

class ECRTExtException : public EOSExtException
{
public:
  static bool classof(const Exception * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_ECRTExtException;
  }
public:
  ECRTExtException();
  explicit ECRTExtException(UnicodeString Msg);
};

class EFatal : public ExtException
{
public:
  static inline bool classof(const Exception * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_EFatal ||
      Obj->GetKind() == OBJECT_CLASS_ESshFatal ||
      Obj->GetKind() == OBJECT_CLASS_ESshTerminate;
  }
public:
  // fatal errors are always copied, new message is only appended
  explicit EFatal(const Exception * E, UnicodeString Msg, UnicodeString HelpKeyword = L"");
  explicit EFatal(TObjectClassId Kind, const Exception * E, UnicodeString Msg, UnicodeString HelpKeyword = L"");

#if 0
  __property bool ReopenQueried = { read = FReopenQueried, write = FReopenQueried };
#endif // #if 0
  bool GetReopenQueried() const { return FReopenQueried; }
  void SetReopenQueried(bool Value) { FReopenQueried = Value; }

  virtual ExtException * Clone() const;

private:
  void Init(const Exception * E);
  bool FReopenQueried;
};

#define DERIVE_FATAL_EXCEPTION(NAME, BASE) \
  class NAME : public BASE \
  { \
  public: \
    static inline bool classof(const Exception * Obj) { return Obj->GetKind() == OBJECT_CLASS_##NAME; } \
  public: \
    explicit inline NAME(const Exception * E, UnicodeString Msg, UnicodeString HelpKeyword = L"") : BASE(OBJECT_CLASS_##NAME, E, Msg, HelpKeyword) {} \
    virtual ExtException * Clone() const { return new NAME(this, L""); } \
  };

DERIVE_FATAL_EXCEPTION(ESshFatal, EFatal)

// exception that closes application, but displays info message (not error message)
// = close on completion
class ESshTerminate : public EFatal
{
public:
  static inline bool classof(const Exception * Obj)
  {
   return
     Obj->GetKind() == OBJECT_CLASS_ESshTerminate;
  }
public:
  explicit inline ESshTerminate(
    const Exception * E, UnicodeString Msg, TOnceDoneOperation AOperation,
    UnicodeString ATargetLocalPath, UnicodeString ADestLocalFileName) :
    EFatal(OBJECT_CLASS_ESshTerminate, E, Msg),
    Operation(AOperation),
    TargetLocalPath(ATargetLocalPath),
    DestLocalFileName(ADestLocalFileName)
  {
  }

  virtual ExtException * Clone() const;
  virtual void Rethrow();

  TOnceDoneOperation Operation;
  UnicodeString TargetLocalPath;
  UnicodeString DestLocalFileName;
};

class ECallbackGuardAbort : public EAbort
{
public:
  static inline bool classof(const Exception * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_ECallbackGuardAbort;
  }
public:
  ECallbackGuardAbort();
};

Exception * CloneException(Exception * E);
void RethrowException(Exception * E);
UnicodeString GetExceptionHelpKeyword(const Exception* E);
UnicodeString MergeHelpKeyword(UnicodeString PrimaryHelpKeyword, UnicodeString SecondaryHelpKeyword);
bool IsInternalErrorHelpKeyword(UnicodeString HelpKeyword);

