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

class NB_CORE_EXPORT ExtException : public Exception
{
public:
  static bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_ExtException); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_ExtException) || Exception::is(Kind); }
public:
  ExtException() = default;
  virtual ~ExtException() noexcept override;
  explicit ExtException(const Exception * E);
  explicit ExtException(TObjectClassId Kind, const Exception * E);
  explicit ExtException(const Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword = L"");
  explicit ExtException(TObjectClassId Kind, const UnicodeString & Msg, const Exception * E);
  explicit ExtException(TObjectClassId Kind, const Exception * E, int32_t Ident);
  // "copy the exception", just append message to the end
  // explicit ExtException(TObjectClassId Kind, const UnicodeString & Msg, const Exception * E, const UnicodeString & HelpKeyword = L"");
  explicit ExtException(const UnicodeString & Msg, const UnicodeString & MoreMessages);
  explicit ExtException(TObjectClassId Kind, const UnicodeString & Msg, const UnicodeString & MoreMessages);
  explicit ExtException(const UnicodeString & Msg, TStrings * MoreMessages, bool Own, const UnicodeString & HelpKeyword = L"");
  __property TStrings * MoreMessages = {read=FMoreMessages};
  __property UnicodeString HelpKeyword = {read=FHelpKeyword};
  TStrings * GetMoreMessages() const { return FMoreMessages; }
  UnicodeString GetHelpKeyword() const { return FHelpKeyword; }

  explicit ExtException(const UnicodeString & Msg) : ExtException(OBJECT_CLASS_ExtException, Msg, nullptr) {}
  explicit ExtException(TObjectClassId Kind, const UnicodeString & Msg) : ExtException(Kind, Msg, nullptr) {}
  explicit ExtException(TObjectClassId Kind, int32_t Ident) : ExtException(Kind, nullptr, Ident) {}
  explicit ExtException(TObjectClassId Kind, const UnicodeString & Msg, int32_t AHelpContext) : Exception(Kind, Msg, AHelpContext) {}

  ExtException(const ExtException & E) : Exception(OBJECT_CLASS_ExtException, L""), FHelpKeyword(E.FHelpKeyword)
  {
    AddMoreMessages(&E);
  }

  ExtException & operator =(const ExtException & rhs)
  {
    if (this != &rhs)
    {
      FHelpKeyword = rhs.FHelpKeyword;
      Message = rhs.Message;
      AddMoreMessages(&rhs);
    }
    return *this;
  }

  static ExtException * CloneFrom(const Exception * E);

  virtual ExtException * Clone() const;
  virtual void Rethrow();

protected:
  void AddMoreMessages(const Exception * E);

private:
  gsl::owner<TStrings *> FMoreMessages{nullptr};
  UnicodeString FHelpKeyword;
};

#define EXT_EXCEPTION_METHODS(NAME, BASE) \
  public: \
    static bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_##NAME); } \
    virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_##NAME) || BASE::is(Kind); } \
  public: \
    using BASE::BASE; \
    explicit NAME(const UnicodeString & Msg, const UnicodeString & MoreMessages) : \
      BASE(OBJECT_CLASS_##NAME, Msg, MoreMessages) \
    { \
    } \
    explicit NAME(const UnicodeString & Msg) : \
      BASE(OBJECT_CLASS_##NAME, Msg) \
    { \
    } \
    virtual ~NAME(void) noexcept override \
    { \
    } \
    virtual ExtException * Clone() const override \
    { \
      return new NAME(OBJECT_CLASS_##NAME, static_cast<const BASE *>(this)); \
    } \
    virtual void Rethrow() override \
    { \
      throw NAME(OBJECT_CLASS_##NAME, static_cast<const BASE *>(this)); \
    }

#define DERIVE_EXT_EXCEPTION(NAME, BASE) \
  extern const TObjectClassId OBJECT_CLASS_##NAME; \
  class NB_CORE_EXPORT NAME : public BASE \
  { \
    EXT_EXCEPTION_METHODS(NAME, BASE) \
  }

DERIVE_EXT_EXCEPTION(ESsh, ExtException);
DERIVE_EXT_EXCEPTION(ETerminal, ExtException);
DERIVE_EXT_EXCEPTION(ECommand, ExtException);
DERIVE_EXT_EXCEPTION(EScp, ExtException); // SCP protocol fatal error (non-fatal in application context)
class NB_CORE_EXPORT ESkipFile : public ExtException
{
public:
  inline ESkipFile() :
    ExtException(OBJECT_CLASS_ESkipFile, UnicodeString(), nullptr)
  {
  }
  EXT_EXCEPTION_METHODS(ESkipFile, ExtException)
};

class NB_CORE_EXPORT EOSExtException : public ExtException
{
public:
  static bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_EOSExtException); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_EOSExtException) || ExtException::is(Kind); }
public:
  explicit EOSExtException();
  explicit EOSExtException(const UnicodeString & Msg);
  explicit EOSExtException(const UnicodeString & Msg, int32_t LastError);
  explicit EOSExtException(TObjectClassId Kind, const UnicodeString & Msg, int32_t LastError);
};

class NB_CORE_EXPORT ECRTExtException : public EOSExtException
{
public:
  static bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_ECRTExtException); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_ECRTExtException) || EOSExtException::is(Kind); }
public:
  ECRTExtException() = default;
  explicit ECRTExtException(const UnicodeString & Msg);
};

class NB_CORE_EXPORT EFatal : public ExtException
{
public:
  static bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_EFatal); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_EFatal) || ExtException::is(Kind); }
public:
  // fatal errors are always copied, new message is only appended
  explicit EFatal(const Exception * E, const UnicodeString & Msg) : EFatal(OBJECT_CLASS_EFatal, Msg, E) {}
  explicit EFatal(TObjectClassId Kind, const UnicodeString & Msg, const Exception * E);

  __property bool ReopenQueried = { read = FReopenQueried, write = FReopenQueried };

  virtual ExtException * Clone() const override;
  virtual void Rethrow() override;

  bool GetReopenQueried() const { return FReopenQueried; }
  void SetReopenQueried(bool Value) { FReopenQueried = Value; }
private:
  bool FReopenQueried{false};
};

#define DERIVE_FATAL_EXCEPTION(NAME, BASE) \
  extern const TObjectClassId OBJECT_CLASS_##NAME; \
  class NB_CORE_EXPORT NAME : public BASE \
  { \
  public: \
    static bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_##NAME); } \
    virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_##NAME) || BASE::is(Kind); } \
  public: \
    explicit inline NAME(const Exception * E, const UnicodeString & Msg) : BASE(OBJECT_CLASS_##NAME, Msg, E) {} \
    virtual ExtException * Clone() const override { return new NAME(this, L""); } \
  }

DERIVE_FATAL_EXCEPTION(EConnectionFatal, EFatal);

// exception that closes application, but displays info message (not error message)
// = close on completion
class NB_CORE_EXPORT ETerminate : public EFatal
{
public:
  static bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_ETerminate); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_ETerminate) || EFatal::is(Kind); }
public:
  explicit ETerminate(
    const Exception * E, const UnicodeString & Msg, TOnceDoneOperation AOperation,
    const UnicodeString & ATargetLocalPath, const UnicodeString & ADestLocalFileName) :
    EFatal(OBJECT_CLASS_ETerminate, Msg, E),
    Operation(AOperation),
    TargetLocalPath(ATargetLocalPath),
    DestLocalFileName(ADestLocalFileName)
  {
  }

  virtual ExtException * Clone() const override;
  virtual void Rethrow() override;

  TOnceDoneOperation Operation;
  UnicodeString TargetLocalPath;
  UnicodeString DestLocalFileName;
};

class NB_CORE_EXPORT ECallbackGuardAbort final : public EAbort
{
public:
  static bool classof(const Exception * Obj) { return Obj && Obj->is(OBJECT_CLASS_ECallbackGuardAbort); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_ECallbackGuardAbort) || EAbort::is(Kind); }
public:
  ECallbackGuardAbort();
};

class EStreamError : public ExtException
{
public:
  static bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_EStreamError); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_EStreamError) || ExtException::is(Kind); }
public:
  explicit EStreamError(TObjectClassId Kind, const UnicodeString & Msg, const Exception * E) : ExtException(Kind, Msg, E) {}
  explicit EStreamError(const UnicodeString & Msg) :
    EStreamError(OBJECT_CLASS_EStreamError, Msg, static_cast<const Exception *>(nullptr))
  {
  }
};

class NB_CORE_EXPORT EFCreateError final : public EStreamError
{
public:
  static bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_EFCreateError); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_EFCreateError) || EStreamError::is(Kind); }
public:
  explicit EFCreateError(const UnicodeString & Msg) :
    EStreamError(OBJECT_CLASS_EFCreateError, Msg, static_cast<const Exception *>(nullptr))
  {
  }
};

class NB_CORE_EXPORT EFOpenError final : public EStreamError
{
public:
  static bool classof(const Exception * Obj) { return Obj->is(OBJECT_CLASS_EFOpenError); }
  virtual bool is(TObjectClassId Kind) const override { return (Kind == OBJECT_CLASS_EFOpenError) || EStreamError::is(Kind); }
public:
  explicit EFOpenError(const UnicodeString & Msg) :
    EStreamError(OBJECT_CLASS_EFOpenError, Msg, static_cast<const Exception *>(nullptr))
  {
  }
};

inline void Abort(const UnicodeString & Message = "")
{
  throw EAbort(Message);
}

inline void Error(int32_t Id, int32_t ErrorId)
{
  const UnicodeString Msg = FMTLOAD(Id, ErrorId);
  throw ExtException(static_cast<Exception *>(nullptr), Msg);
}

inline void ThrowNotImplemented(int32_t ErrorId)
{
  Error(SNotImplemented, ErrorId);
}

constexpr const auto * EXCEPTION_MSG_REPLACED = L"[replaced]";

template<typename From, typename To>
inline void TryReplaceAndThrow(Exception & E)
{
  if (rtti::isa<From>(&E))
  {
    throw To(EXCEPTION_MSG_REPLACED);
  }
  throw; //NOSONAR
}

NB_CORE_EXPORT Exception * CloneException(Exception * E);
NB_CORE_EXPORT void RethrowException(Exception * E);
NB_CORE_EXPORT UnicodeString GetExceptionHelpKeyword(const Exception * E);
NB_CORE_EXPORT UnicodeString MergeHelpKeyword(const UnicodeString & PrimaryHelpKeyword, const UnicodeString & SecondaryHelpKeyword);
NB_CORE_EXPORT bool IsInternalErrorHelpKeyword(const UnicodeString & HelpKeyword);
UnicodeString AddContextToExceptionMessage(const Exception & E, const UnicodeString & NewContext);

