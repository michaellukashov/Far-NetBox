#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include <StrUtils.hpp>

#include "TextsCore.h"
#if defined(FARPLUGIN)
#include "HelpCore.h"
#endif // FARPLUGIN
#include "rtlconsts.h"

//static std::unique_ptr<TCriticalSection> IgnoredExceptionsCriticalSection(new TCriticalSection());
//typedef rde::set<UnicodeString> TIgnoredExceptions;
//static TIgnoredExceptions IgnoredExceptions;

//static UnicodeString NormalizeClassName(const UnicodeString & ClassName)
//{
//  return ReplaceStr(ClassName, L".", L"::").LowerCase();
//}

//void IgnoreException(const std::type_info & ExceptionType)
//{
//  TGuard Guard(*IgnoredExceptionsCriticalSection.get());
//  // We should better use type_index as a key, instead of a class name,
//  // but type_index is not available in 32-bit version of STL in XE6.
//  IgnoredExceptions.insert(NormalizeClassName(UnicodeString(AnsiString(ExceptionType.name()))));
//}

static bool WellKnownException(
  const Exception * E, UnicodeString * AMessage, const wchar_t ** ACounterName, Exception ** AClone, bool Rethrow)
{
  UnicodeString Message;
  const wchar_t * CounterName = nullptr;
  std::unique_ptr<Exception> Clone;


  bool Result = true;
  bool IgnoreException = false;

//  if (!IgnoredExceptions.empty())
//  {
//    TGuard Guard(*IgnoredExceptionsCriticalSection.get());
//    UnicodeString ClassName = ""; // NormalizeClassName(E->QualifiedClassName());
//    IgnoreException = (IgnoredExceptions.find(ClassName) != IgnoredExceptions.end());
//  }

  if (IgnoreException)
  {
    Result = false;
  }
  // EAccessViolation is EExternal
  else if (isa<EAccessViolation>(E))
  {
    if (Rethrow)
    {
      throw EAccessViolation(E->Message);
    }
    Message = MainInstructions(LoadStr(ACCESS_VIOLATION_ERROR3));
    CounterName = L"AccessViolations";
    Clone.reset(new EAccessViolation(E->Message));
  }
  /*
  // EIntError and EMathError are EExternal
  // EClassNotFound is EFilerError
  else if ((dyn_cast<EListError>(E) != nullptr) ||
           (dyn_cast<EStringListError>(E) != nullptr) ||
           (dyn_cast<EIntError>(E) != nullptr) ||
           (dyn_cast<EMathError>(E) != nullptr) ||
           (dyn_cast<EVariantError>(E) != nullptr) ||
           (dyn_cast<EInvalidOperation>(E) != nullptr))
           (dynamic_cast<EFilerError*>(E) != NULL))
  {
    if (Rethrow)
    {
      throw EIntError(E->Message);
    }
    Message = MainInstructions(E->Message);
    CounterName = L"InternalExceptions";
    Clone.reset(new EIntError(E->Message));
  }
  else if (dyn_cast<EExternal>(E) != nullptr)
  {
    if (Rethrow)
    {
      throw EExternal(E->Message);
    }
    Message = MainInstructions(E->Message);
    CounterName = L"ExternalExceptions";
    Clone.reset(new EExternal(E->Message));
  }
  else if (dyn_cast<EHeapException>(E) != nullptr)
  {
    if (Rethrow)
    {
      throw EHeapException(E->Message);
    }
    Message = MainInstructions(E->Message);
    CounterName = L"HeapExceptions";
    Clone.reset(new EHeapException(E->Message));
  }
  */
  else
  {
    Result = false;
  }

  if (Result)
  {
    if (AMessage != nullptr)
    {
      (*AMessage) = Message;
    }
    if (ACounterName != nullptr)
    {
      (*ACounterName) = CounterName;
    }
    if (AClone != nullptr)
    {
      (*AClone) = DebugNotNull(Clone.release());
    }
  }

  return Result;
}

static bool ExceptionMessage(const Exception * E, bool /*Count*/,
  bool Formatted, UnicodeString & Message, bool & InternalError)
{
  bool Result = true;
  const wchar_t * CounterName = nullptr;
  InternalError = false; // see also IsInternalException

  // this list has to be in sync with CloneException
  if (isa<EAbort>(E))
  {
    Result = false;
  }
  else if (WellKnownException(E, &Message, &CounterName, nullptr, false))
  {
    InternalError = true;
  }
  else if (E && E->Message.IsEmpty())
  {
    Result = false;
  }
  else if (E)
  {
    Message = E->Message;
  }

  if (!Formatted)
  {
    Message = UnformatMessage(Message);
  }

  if (InternalError)
  {
    Message = FMTLOAD(REPORT_ERROR, Message.c_str());
  }
/*
  if (Count && (CounterName != nullptr) && (Configuration->Usage != nullptr))
  {
    Configuration->Usage->Inc(CounterName);
    UnicodeString ExceptionDebugInfo =
      E->ClassName() + L":" + GetExceptionDebugInfo();
    Configuration->Usage->Set(LastInternalExceptionCounter, ExceptionDebugInfo);
  }
*/
  return Result;
}

bool IsInternalException(const Exception * E)
{
  // see also InternalError in ExceptionMessage
  return WellKnownException(E, nullptr, nullptr, nullptr, false);
}

bool ExceptionMessage(const Exception * E, UnicodeString & Message)
{
  bool InternalError;
  return ExceptionMessage(E, true, false, Message, InternalError);
}

bool ExceptionMessageFormatted(const Exception * E, UnicodeString & Message)
{
  bool InternalError;
  return ExceptionMessage(E, true, true, Message, InternalError);
}

bool ShouldDisplayException(Exception * E)
{
  UnicodeString Message;
  return ExceptionMessageFormatted(E, Message);
}

TStrings * ExceptionToMoreMessages(Exception * E)
{
  TStrings * Result = nullptr;
  UnicodeString Message;
  if (ExceptionMessage(E, Message))
  {
    Result = new TStringList();
    Result->Add(Message);
    ExtException * ExtE = dyn_cast<ExtException>(E);
    if ((ExtE != nullptr) && (ExtE->GetMoreMessages() != nullptr))
    {
      Result->AddStrings(ExtE->GetMoreMessages());
    }
  }
  return Result;
}

bool ExceptionFullMessage(Exception * E, UnicodeString & Message)
{
  bool Result = ExceptionMessage(E, Message);
  if (Result)
  {
    Message += L"\n";
    ExtException * EE = dyn_cast<ExtException>(E);
    if ((EE != nullptr) && (EE->GetMoreMessages() != nullptr))
    {
      Message += EE->GetMoreMessages()->GetText() + L"\n";
    }
  }
  return Result;
}

UnicodeString GetExceptionHelpKeyword(const Exception * E)
{
  UnicodeString HelpKeyword;
  const ExtException * ExtE = dyn_cast<ExtException>(E);
  UnicodeString Message; // not used
  bool InternalError = false;
  if (ExtE != nullptr)
  {
    HelpKeyword = ExtE->GetHelpKeyword();
  }
  else if ((E != nullptr) && ExceptionMessage(E, false, false, Message, InternalError) &&
           InternalError)
  {
#if defined(FARPLUGIN)
    HelpKeyword = HELP_INTERNAL_ERROR;
#endif // FARPLUGIN
  }
  return HelpKeyword;
}

UnicodeString MergeHelpKeyword(const UnicodeString & PrimaryHelpKeyword, const UnicodeString & SecondaryHelpKeyword)
{
  if (!PrimaryHelpKeyword.IsEmpty() &&
      !IsInternalErrorHelpKeyword(SecondaryHelpKeyword))
  {
    // we have to yet decide what we have both
    // PrimaryHelpKeyword and SecondaryHelpKeyword
    return PrimaryHelpKeyword;
  }
  else
  {
    return SecondaryHelpKeyword;
  }
}

bool IsInternalErrorHelpKeyword(const UnicodeString & HelpKeyword)
{
#if defined(FARPLUGIN)
  return
    (HelpKeyword == HELP_INTERNAL_ERROR);
#else
  return false;
#endif // FARPLUGIN
}

ExtException::ExtException(Exception * E) :
  Exception(OBJECT_CLASS_ExtException, L""),
  FMoreMessages(nullptr)
{
  AddMoreMessages(E);
  FHelpKeyword = GetExceptionHelpKeyword(E);
}

ExtException::ExtException(TObjectClassId Kind, Exception * E) :
  Exception(Kind, L""),
  FMoreMessages(nullptr)
{
  AddMoreMessages(E);
  FHelpKeyword = GetExceptionHelpKeyword(E);
}

ExtException::ExtException(const Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword) :
  Exception(OBJECT_CLASS_ExtException, Msg),
  FMoreMessages(nullptr)
{
  AddMoreMessages(E);
  FHelpKeyword = MergeHelpKeyword(HelpKeyword, GetExceptionHelpKeyword(E));
}

ExtException::ExtException(TObjectClassId Kind, const Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword) :
  Exception(Kind, Msg),
  FMoreMessages(nullptr)
{
  AddMoreMessages(E);
  FHelpKeyword = MergeHelpKeyword(HelpKeyword, GetExceptionHelpKeyword(E));
}
/*ExtException::ExtException(TObjectClassId Kind, ExtException * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword) :
  Exception(Kind, Msg),
  FMoreMessages(nullptr),
  FHelpKeyword()
{
  AddMoreMessages(E);
}*/

ExtException::ExtException(TObjectClassId Kind, Exception * E, intptr_t Ident) :
  Exception(Kind, E, Ident),
  FMoreMessages(nullptr),
  FHelpKeyword()
{
}

ExtException::ExtException(TObjectClassId Kind, const UnicodeString & Msg, const Exception * E, const UnicodeString & HelpKeyword) :
  Exception(Kind, L""),
  FMoreMessages(nullptr)
{
  // "copy exception"
  AddMoreMessages(E);
  // and append message to the end to more messages
  if (!Msg.IsEmpty())
  {
    if (Message.IsEmpty())
    {
      Message = Msg;
    }
    else
    {
      if (FMoreMessages == nullptr)
      {
        FMoreMessages = new TStringList();
      }
      FMoreMessages->Append(UnformatMessage(Msg));
    }
  }
  FHelpKeyword = MergeHelpKeyword(GetExceptionHelpKeyword(E), HelpKeyword);
}

ExtException::ExtException(const UnicodeString & Msg, const UnicodeString & MoreMessages,
  const UnicodeString & HelpKeyword) :
  Exception(OBJECT_CLASS_ExtException, Msg),
  FMoreMessages(nullptr),
  FHelpKeyword(HelpKeyword)
{
  if (!MoreMessages.IsEmpty())
  {
    FMoreMessages = TextToStringList(MoreMessages);
  }
}

ExtException::ExtException(TObjectClassId Kind, const UnicodeString & Msg, const UnicodeString & MoreMessages,
  const UnicodeString & HelpKeyword) :
  Exception(Kind, Msg),
  FMoreMessages(nullptr),
  FHelpKeyword(HelpKeyword)
{
  if (!MoreMessages.IsEmpty())
  {
    FMoreMessages = TextToStringList(MoreMessages);
  }
}

ExtException::ExtException(const UnicodeString & Msg, TStrings * MoreMessages,
  bool Own, const UnicodeString & HelpKeyword) :
  Exception(OBJECT_CLASS_ExtException, Msg),
  FMoreMessages(nullptr),
  FHelpKeyword(HelpKeyword)
{
  if (Own)
  {
    FMoreMessages = MoreMessages;
  }
  else
  {
    FMoreMessages = new TStringList();
    FMoreMessages->Assign(MoreMessages);
  }
}

void ExtException::AddMoreMessages(const Exception * E)
{
  if (E != nullptr)
  {
    if (FMoreMessages == nullptr)
    {
      FMoreMessages = new TStringList();
    }

    const ExtException * ExtE = dyn_cast<ExtException>(E);
    if (ExtE != nullptr)
    {
      if (ExtE->GetMoreMessages() != nullptr)
      {
        FMoreMessages->Assign(ExtE->GetMoreMessages());
      }
    }

    UnicodeString Msg;
    ExceptionMessageFormatted(E, Msg);

    // new exception does not have own message, this is in fact duplication of
    // the exception data, but the exception class may being changed
    if (Message.IsEmpty())
    {
      Message = Msg;
    }
    else if (!Msg.IsEmpty())
    {
      FMoreMessages->Insert(0, UnformatMessage(Msg));
    }

    if (IsInternalException(E))
    {
      AppendExceptionStackTraceAndForget(FMoreMessages);
    }

    if (FMoreMessages->GetCount() == 0)
    {
      SAFE_DESTROY(FMoreMessages);
    }
  }
}

ExtException::~ExtException() noexcept
{
  SAFE_DESTROY(FMoreMessages);
}

ExtException * ExtException::CloneFrom(const Exception * E)
{
  return new ExtException(E, L"");
}

ExtException * ExtException::Clone() const
{
  return CloneFrom(this);
}

UnicodeString SysErrorMessageForError(intptr_t LastError)
{
  UnicodeString Result;
  if (LastError != 0)
  {
    //Result = FORMAT("System Error. Code: %d.\r\n%s", LastError, SysErrorMessage(LastError));
    Result = FMTLOAD(SOSError, LastError, ::SysErrorMessage(LastError), L"");
  }
  return Result;
}

UnicodeString LastSysErrorMessage()
{
  return SysErrorMessageForError(GetLastError());
}

EOSExtException::EOSExtException() :
  ExtException(OBJECT_CLASS_EOSExtException, nullptr, 0)
{
}

EOSExtException::EOSExtException(const UnicodeString & Msg) :
  ExtException(OBJECT_CLASS_EOSExtException, Msg, LastSysErrorMessage())
{
}

EOSExtException::EOSExtException(const UnicodeString & Msg, intptr_t LastError) :
  ExtException(OBJECT_CLASS_EOSExtException, Msg, SysErrorMessageForError(LastError))
{
}

EOSExtException::EOSExtException(TObjectClassId Kind, const UnicodeString & Msg, intptr_t LastError) :
  ExtException(Kind, Msg, SysErrorMessageForError(LastError))
{
}

EFatal::EFatal(const Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword) :
  ExtException(OBJECT_CLASS_EFatal, Msg, E, HelpKeyword),
  FReopenQueried(false)
{
  Init(E);
}

EFatal::EFatal(TObjectClassId Kind, const Exception * E, const UnicodeString & Msg, const UnicodeString & HelpKeyword) :
  ExtException(Kind, Msg, E, HelpKeyword),
  FReopenQueried(false)
{
  Init(E);
}

ExtException * EFatal::Clone() const
{
  return new EFatal(OBJECT_CLASS_EFatal, this, L"");
}

void EFatal::Init(const Exception * E)
{
  const EFatal * F = dyn_cast<EFatal>(E);
  if (F != nullptr)
  {
    FReopenQueried = F->GetReopenQueried();
  }
}

ECRTExtException::ECRTExtException(const UnicodeString & Msg) :
  EOSExtException(OBJECT_CLASS_ECRTExtException, Msg, errno)
{
}

ExtException * ESshTerminate::Clone() const
{
  return new ESshTerminate(this, L"", Operation);
}

ECallbackGuardAbort::ECallbackGuardAbort() : EAbort(OBJECT_CLASS_ECallbackGuardAbort, L"callback abort")
{
}

Exception * CloneException(Exception * E)
{
  Exception * Result;
  // this list has to be in sync with ExceptionMessage
  ExtException * Ext = dyn_cast<ExtException>(E);
  if (Ext != nullptr)
  {
    Result = Ext->Clone();
  }
  else if (isa<ECallbackGuardAbort>(E))
  {
    Result = new ECallbackGuardAbort();
  }
  else if (isa<EAbort>(E))
  {
    Result = new EAbort(E->Message);
  }
  else if (WellKnownException(E, nullptr, nullptr, &Result, false))
  {
    // noop
  }
  else
  {
    // we do not expect this to happen
    if (DebugAlwaysFalse(IsInternalException(E)))
    {
      // to save exception stack trace
      Result = ExtException::CloneFrom(E);
    }
    else
    {
      Result = new Exception(E->Message);
    }
  }
  return Result;
}

void RethrowException(Exception * E)
{
  // this list has to be in sync with ExceptionMessage
  if (isa<EFatal>(E))
  {
    throw EFatal(E, L"");
  }
  else if (isa<ECallbackGuardAbort>(E))
  {
    throw ECallbackGuardAbort();
  }
  else if (isa<EAbort>(E))
  {
    throw EAbort(E->Message);
  }
  else if (WellKnownException(E, nullptr, nullptr, nullptr, true))
  {
    // noop, should never get here
  }
  else
  {
    throw ExtException(E, L"");
  }
}

