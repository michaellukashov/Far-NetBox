
#pragma once

#include <Classes.hpp>
#include <Sysutils.hpp>
#include <SysInit.hpp>
#include <System.hpp>

NB_CORE_EXPORT bool ShouldDisplayException(Exception * E);
NB_CORE_EXPORT bool ExceptionMessage(const Exception * E, UnicodeString & Message);
NB_CORE_EXPORT bool ExceptionMessageFormatted(const Exception * E, UnicodeString & Message);
NB_CORE_EXPORT bool ExceptionFullMessage(Exception * E, UnicodeString & Message);
NB_CORE_EXPORT UnicodeString SysErrorMessageForError(int32_t LastError);
NB_CORE_EXPORT UnicodeString LastSysErrorMessage();
NB_CORE_EXPORT TStrings * ExceptionToMoreMessages(Exception * E);
NB_CORE_EXPORT bool IsInternalException(const Exception * E);

enum TOnceDoneOperation { odoIdle, odoDisconnect, odoSuspend, odoShutDown };

NB_DEFINE_CLASS_ID(ExtException);
class NB_CORE_EXPORT ExtException : public Exception
{
public:
  static bool classof(const Exception *Obj) { return Obj->is(OBJECT_CLASS_ExtException); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_ExtException) || Exception::is(Kind); }
public:
  ExtException() = default;
  virtual ~ExtException() noexcept;
  explicit ExtException(const Exception * E);
  explicit ExtException(TObjectClassId Kind, const Exception * E);
  explicit ExtException(const Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"");
  explicit ExtException(TObjectClassId Kind, const Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"");
  // explicit ExtException(const ExtException *E, const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"");
  explicit ExtException(TObjectClassId Kind, Exception * E, int32_t Ident, const UnicodeString & HelpKeyword = L"");
  // "copy the exception", just append message to the end
  explicit ExtException(TObjectClassId Kind, const UnicodeString & Msg, const Exception * E, const UnicodeString & HelpKeyword = L"");
  explicit ExtException(const UnicodeString & Msg, const UnicodeString & MoreMessages, const UnicodeString & HelpKeyword = L"");
  explicit ExtException(TObjectClassId Kind, const UnicodeString & Msg, const UnicodeString & MoreMessages, const UnicodeString & HelpKeyword = L"");
  explicit ExtException(const UnicodeString & Msg, TStrings *MoreMessages, bool Own, const UnicodeString & HelpKeyword = L"");
  __property TStrings * MoreMessages = {read=FMoreMessages};
  __property UnicodeString HelpKeyword = {read=FHelpKeyword};
  TStrings * GetMoreMessages() const { return FMoreMessages; }
  UnicodeString GetHelpKeyword() const { return FHelpKeyword; }

  explicit ExtException(const UnicodeString & Msg) : Exception(OBJECT_CLASS_ExtException, Msg) {}
  explicit ExtException(TObjectClassId Kind, const UnicodeString & Msg) : Exception(Kind, Msg) {}
  explicit ExtException(TObjectClassId Kind, int32_t Ident) : Exception(Kind, Ident) {}
  explicit ExtException(TObjectClassId Kind, const UnicodeString & Msg, int32_t AHelpContext) : Exception(Kind, Msg, AHelpContext) {}

  ExtException(const ExtException& E) : Exception(OBJECT_CLASS_ExtException, L""), FHelpKeyword(E.FHelpKeyword)
  {
    AddMoreMessages(&E);
  }
  ExtException &operator=(const ExtException &rhs)
  {
    FHelpKeyword = rhs.FHelpKeyword;
    Message = rhs.Message;
    AddMoreMessages(&rhs);
    return *this;
  }

  static ExtException * CloneFrom(const Exception *E);

  virtual ExtException * Clone() const;
  virtual void Rethrow();

protected:
  void AddMoreMessages(const Exception * E);

private:
  TStrings * FMoreMessages{nullptr};
  UnicodeString FHelpKeyword;
};

#define EXT_EXCEPTION_METHODS(NAME, BASE) \
  public: \
    static bool classof(const Exception *Obj) { return Obj->is(OBJECT_CLASS_##NAME); } \
    bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_##NAME) || BASE::is(Kind); } \
  public: \
    explicit inline NAME(const Exception *E, const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"") : \
      BASE(OBJECT_CLASS_##NAME, E, Msg, HelpKeyword) \
    { \
    } \
    explicit inline NAME(TObjectClassId Kind, const Exception *E, const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"") : \
      BASE(Kind, E, Msg, HelpKeyword) \
    { \
    } \
    inline NAME(TObjectClassId Kind, const UnicodeString & Msg, const UnicodeString & MoreMessages, const UnicodeString & HelpKeyword = UnicodeString()) : \
      BASE(Kind, Msg, MoreMessages, HelpKeyword) \
    { \
    } \
    virtual inline ~NAME(void) \
    { \
    } \
    explicit inline NAME(TObjectClassId Kind, const UnicodeString & Msg, int32_t AHelpContext) : \
      BASE(Kind, Msg, AHelpContext) \
    { \
    } \
    explicit inline NAME(const UnicodeString & Msg, int32_t AHelpContext) : \
      BASE(OBJECT_CLASS_##NAME, Msg, AHelpContext) \
    { \
    } \
    virtual ExtException * Clone() const override \
    { \
      return new NAME(OBJECT_CLASS_##NAME, this, L""); \
    } \
    virtual void Rethrow() override \
    { \
      throw NAME(OBJECT_CLASS_##NAME, this, L""); \
    }

#define DERIVE_EXT_EXCEPTION(NAME, BASE) \
  NB_DEFINE_CLASS_ID(NAME); \
  class NB_CORE_EXPORT NAME : public BASE \
  { \
    EXT_EXCEPTION_METHODS(NAME, BASE) \
  };

DERIVE_EXT_EXCEPTION(ESsh, ExtException)
DERIVE_EXT_EXCEPTION(ETerminal, ExtException)
DERIVE_EXT_EXCEPTION(ECommand, ExtException)
DERIVE_EXT_EXCEPTION(EScp, ExtException) // SCP protocol fatal error (non-fatal in application context)
NB_DEFINE_CLASS_ID(ESkipFile);
class NB_CORE_EXPORT ESkipFile : public ExtException
{
public:
  inline ESkipFile() :
    ExtException(OBJECT_CLASS_ESkipFile, nullptr, UnicodeString())
  {
  }
  EXT_EXCEPTION_METHODS(ESkipFile, ExtException)
};

NB_DEFINE_CLASS_ID(EOSExtException);
class NB_CORE_EXPORT EOSExtException : public ExtException
{
public:
  static bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_EOSExtException); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_EOSExtException) || ExtException::is(Kind); }
public:
  explicit EOSExtException();
  explicit EOSExtException(const UnicodeString & Msg);
  explicit EOSExtException(const UnicodeString & Msg, int32_t LastError);
  explicit EOSExtException(TObjectClassId Kind, const UnicodeString & Msg, int32_t LastError);
};

NB_DEFINE_CLASS_ID(ECRTExtException);
class NB_CORE_EXPORT ECRTExtException : public EOSExtException
{
public:
  static bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_ECRTExtException); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_ECRTExtException) || EOSExtException::is(Kind); }
public:
  ECRTExtException() = default;
  explicit ECRTExtException(const UnicodeString & Msg);
};

NB_DEFINE_CLASS_ID(EFatal);
class NB_CORE_EXPORT EFatal : public ExtException
{
public:
  static bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_EFatal); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_EFatal) || ExtException::is(Kind); }
public:
  // fatal errors are always copied, new message is only appended
  explicit EFatal(const Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"");
  explicit EFatal(TObjectClassId Kind, const Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"");

  __property bool ReopenQueried = { read = FReopenQueried, write = FReopenQueried };

  virtual ExtException *Clone() const override;
  virtual void Rethrow() override;

  bool GetReopenQueried() const { return FReopenQueried; }
  void SetReopenQueried(bool Value) { FReopenQueried = Value; }
private:
  void Init(const Exception *E);
  bool FReopenQueried;
};

#define DERIVE_FATAL_EXCEPTION(NAME, BASE) \
  NB_DEFINE_CLASS_ID(NAME); \
  class NB_CORE_EXPORT NAME : public BASE \
  { \
  public: \
    static bool classof(const Exception *Obj) { return Obj->is(OBJECT_CLASS_##NAME); } \
    bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_##NAME) || BASE::is(Kind); } \
  public: \
    explicit inline NAME(const Exception *E, const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"") : BASE(OBJECT_CLASS_##NAME, E, Msg, HelpKeyword) {} \
    virtual ExtException *Clone() const override { return new NAME(this, L""); } \
  };

DERIVE_FATAL_EXCEPTION(ESshFatal, EFatal)

// exception that closes application, but displays info message (not error message)
// = close on completion
NB_DEFINE_CLASS_ID(ESshTerminate);
class NB_CORE_EXPORT ESshTerminate : public EFatal
{
public:
  static bool classof(const Exception *Obj) { return Obj->is(OBJECT_CLASS_ESshTerminate); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_ESshTerminate) || EFatal::is(Kind); }
public:
  explicit inline ESshTerminate(
    const Exception * E, const UnicodeString & Msg, TOnceDoneOperation AOperation,
    const UnicodeString & ATargetLocalPath, const UnicodeString & ADestLocalFileName) :
    EFatal(OBJECT_CLASS_ESshTerminate, E, Msg),
    Operation(AOperation),
    TargetLocalPath(ATargetLocalPath),
    DestLocalFileName(ADestLocalFileName)
  {
  }

  virtual ExtException *Clone() const override;
  virtual void Rethrow() override;

  TOnceDoneOperation Operation;
  UnicodeString TargetLocalPath;
  UnicodeString DestLocalFileName;
};

NB_DEFINE_CLASS_ID(ECallbackGuardAbort);
class NB_CORE_EXPORT ECallbackGuardAbort : public EAbort
{
public:
  static bool classof(const Exception *Obj) { return Obj->is(OBJECT_CLASS_ECallbackGuardAbort); }
  bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_ECallbackGuardAbort) || EAbort::is(Kind); }
public:
  ECallbackGuardAbort();
};

NB_DEFINE_CLASS_ID(EStreamError);
class EStreamError : public ExtException
{
public:
  static bool classof(const Exception *Obj) { return Obj->is(OBJECT_CLASS_EStreamError); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_EStreamError) || Exception::is(Kind); }
public:
  using ExtException::ExtException;
  explicit EStreamError(const UnicodeString & Msg) :
      ExtException(OBJECT_CLASS_EStreamError, static_cast<const Exception *>(nullptr), Msg)
  {
  }
};

NB_DEFINE_CLASS_ID(EFCreateError);
class NB_CORE_EXPORT EFCreateError : public EStreamError
{
public:
  static bool classof(const Exception *Obj) { return Obj->is(OBJECT_CLASS_EFCreateError); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_EFCreateError) || Exception::is(Kind); }
public:
  explicit EFCreateError(const UnicodeString & Msg) :
      EStreamError(OBJECT_CLASS_EFCreateError, static_cast<const Exception *>(nullptr), Msg)
  {
  }
};

NB_DEFINE_CLASS_ID(EFOpenError);
class NB_CORE_EXPORT EFOpenError : public EStreamError
{
public:
  static bool classof(const Exception *Obj) { return Obj->is(OBJECT_CLASS_EFOpenError); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_EFOpenError) || Exception::is(Kind); }
public:
  explicit EFOpenError(const UnicodeString & Msg) :
      EStreamError(OBJECT_CLASS_EFOpenError, static_cast<const Exception *>(nullptr), Msg)
  {
  }
};

inline void Abort()
{
  throw EAbort("");
}

inline void Error(int32_t Id, int32_t ErrorId)
{
  UnicodeString Msg = FMTLOAD(Id, ErrorId);
  throw ExtException(static_cast<Exception *>(nullptr), Msg);
}

inline void ThrowNotImplemented(int32_t ErrorId)
{
  Error(SNotImplemented, ErrorId);
}

NB_CORE_EXPORT Exception * CloneException(Exception * E);
NB_CORE_EXPORT void RethrowException(Exception * E);
NB_CORE_EXPORT UnicodeString GetExceptionHelpKeyword(const Exception * E);
NB_CORE_EXPORT UnicodeString MergeHelpKeyword(const UnicodeString & PrimaryHelpKeyword, const UnicodeString & SecondaryHelpKeyword);
NB_CORE_EXPORT bool IsInternalErrorHelpKeyword(const UnicodeString & HelpKeyword);
UnicodeString AddContextToExceptionMessage(const Exception & E, const UnicodeString & NewContext);

