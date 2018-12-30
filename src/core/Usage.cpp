//---------------------------------------------------------------------------
#include <vcl.h>
__removed #pragma hdrstop

#include <Configuration.h>
#include <CoreMain.h>
#include <Common.h>
#include <Usage.h>
//---------------------------------------------------------------------------
__removed #pragma package(smart_init)
//---------------------------------------------------------------------------
const UnicodeString LastInternalExceptionCounter(L"LastInternalException2");
const UnicodeString LastUpdateExceptionCounter(L"LastUpdateException");
//---------------------------------------------------------------------------
TUsage::TUsage(TConfiguration * Configuration) noexcept
{
  FCriticalSection = std::make_unique<TCriticalSection>();
  FConfiguration = Configuration;
  FValues = std::make_unique<TStringList>();
  FValues->SetDelimiter(L'&');
  FValues->SetStrictDelimiter(true);
  FCollect = true;
  Default();
}
//---------------------------------------------------------------------------
TUsage::~TUsage() noexcept
{
  __removed delete FValues;
  __removed delete FCriticalSection;
}
//---------------------------------------------------------------------------
void TUsage::Default()
{
  TGuard Guard(*FCriticalSection); nb::used(Guard);
  FPeriodCounters.clear();
  FLifetimeCounters.clear();
  FValues->Clear();

  if (FCollect) // optimization
  {
    Set(L"FirstUse", StandardTimestamp());
    Set(L"FirstVersion", IntToStr(FConfiguration->CompoundVersion));
    UpdateLastReport();
  }
}
//---------------------------------------------------------------------------
void TUsage::Load(THierarchicalStorage * Storage)
{
  TGuard Guard(*FCriticalSection); nb::used(Guard);
  Default();

  if (Storage->OpenSubKey(L"Values", false))
  {
    std::unique_ptr<TStrings> Names = std::make_unique<TStringList>();
    try__finally
    {
      Storage->GetValueNames(Names.get());
      for (int Index = 0; Index < Names->Count; Index++)
      {
        UnicodeString Name = Names->GetString(Index);
        Set(Name, Storage->ReadString(Name, L""));
      }
      Storage->CloseSubKey();
    },
    __finally__removed
    ({
      delete Names;
    }) end_try__finally
  }

  Load(Storage, L"PeriodCounters", FPeriodCounters);
  Load(Storage, L"LifetimeCounters", FLifetimeCounters);
}
//---------------------------------------------------------------------------
void TUsage::Load(THierarchicalStorage * Storage,
  const UnicodeString AName, TCounters & Counters)
{
  if (Storage->OpenSubKey(AName, false))
  {
    std::unique_ptr<TStrings> Names = std::make_unique<TStringList>();
    try__finally
    {
      Storage->GetValueNames(Names.get());
      for (int Index = 0; Index < Names->Count; Index++)
      {
        UnicodeString Name = Names->GetString(Index);
        Counters[Name] = Storage->ReadInteger(Name, 0);
      }
      Storage->CloseSubKey();
    },
    __finally__removed
    ({
      delete Names;
    }) end_try__finally
  }
}
//---------------------------------------------------------------------------
void TUsage::Save(THierarchicalStorage * Storage) const
{
  TGuard Guard(*FCriticalSection); nb::used(Guard);
  if (Storage->OpenSubKey("Values", true))
  {
    Storage->ClearValues();
    Storage->WriteValues(FValues.get(), true);
    Storage->CloseSubKey();
  }

  Save(Storage, L"PeriodCounters", FPeriodCounters);
  Save(Storage, L"LifetimeCounters", FLifetimeCounters);
}
//---------------------------------------------------------------------------
void TUsage::Save(THierarchicalStorage * Storage,
  const UnicodeString AName, const TCounters & Counters) const
{
  if (Storage->OpenSubKey(AName, true))
  {
    Storage->ClearValues();
    TCounters::const_iterator i = Counters.begin();
    while (i != Counters.end())
    {
      Storage->WriteInteger(i->first, i->second);
      i++;
    }
    Storage->CloseSubKey();
  }
}
//---------------------------------------------------------------------------
void TUsage::Set(const UnicodeString AKey, const UnicodeString AValue)
{
  if (FCollect)
  {
    TGuard Guard(*FCriticalSection); nb::used(Guard);
    FValues->SetValue(AKey, AValue);
  }
}
//---------------------------------------------------------------------------
void TUsage::Set(const UnicodeString AKey, intptr_t Value)
{
  Set(AKey, IntToStr(Value));
}
//---------------------------------------------------------------------------
void TUsage::Set(const UnicodeString AKey, bool Value)
{
  Set(AKey, intptr_t(Value ? 1 : 0));
}
//---------------------------------------------------------------------------
UnicodeString TUsage::Get(const UnicodeString AKey) const
{
  TGuard Guard(*FCriticalSection); nb::used(Guard);
  UnicodeString Result = FValues->GetValue(AKey);
  Result.Unique();
  return Result;
}
//---------------------------------------------------------------------------
void TUsage::UpdateLastReport()
{
  Set("LastReport", StandardTimestamp());
}
//---------------------------------------------------------------------------
void TUsage::Reset()
{
  TGuard Guard(*FCriticalSection); nb::used(Guard);
  UpdateLastReport();
  FPeriodCounters.clear();
  ResetLastExceptions();
}
//---------------------------------------------------------------------------
void TUsage::UpdateCurrentVersion()
{
  TGuard Guard(*FCriticalSection); nb::used(Guard);
  intptr_t CompoundVersion = FConfiguration->CompoundVersion;
  intptr_t PrevVersion = StrToIntDef(Get("CurrentVersion"), 0);
  if (PrevVersion != CompoundVersion)
  {
    Set("Installed", StandardTimestamp());
  }
  if (PrevVersion != 0)
  {
    if (PrevVersion < CompoundVersion)
    {
      Inc("Upgrades");
    }
    else if (PrevVersion > CompoundVersion)
    {
      Inc("Downgrades");
    }

    if (PrevVersion != CompoundVersion)
    {
      ResetLastExceptions();
    }
  }
  Set("CurrentVersion", CompoundVersion);
}
//---------------------------------------------------------------------------
void TUsage::ResetValue(const UnicodeString AKey)
{
  intptr_t Index = FValues->IndexOfName(AKey);
  if (Index >= 0)
  {
    FValues->Delete(Index);
  }
}
//---------------------------------------------------------------------------
void TUsage::ResetLastExceptions()
{
  TGuard Guard(*FCriticalSection); nb::used(Guard);
  ResetValue(LastInternalExceptionCounter);
  ResetValue(LastUpdateExceptionCounter);
}
//---------------------------------------------------------------------------
void TUsage::Inc(const UnicodeString AKey, intptr_t Increment)
{
  if (FCollect)
  {
    TGuard Guard(*FCriticalSection); nb::used(Guard);
    Inc(AKey, FPeriodCounters, Increment);
    Inc(AKey, FLifetimeCounters, Increment);
  }
}
//---------------------------------------------------------------------------
void TUsage::Inc(const UnicodeString AKey, TCounters & Counters, intptr_t Increment)
{
  TCounters::iterator i = Counters.find(AKey);
  if (i != Counters.end())
  {
    i->second += Increment;
  }
  else
  {
    Counters[AKey] = Increment;
  }
}
//---------------------------------------------------------------------------
void TUsage::SetMax(const UnicodeString AKey, intptr_t Value)
{
  if (FCollect)
  {
    TGuard Guard(*FCriticalSection); nb::used(Guard);
    SetMax(AKey, Value, FPeriodCounters);
    SetMax(AKey, Value, FLifetimeCounters);
  }
}
//---------------------------------------------------------------------------
void TUsage::SetMax(const UnicodeString AKey, intptr_t Value,
  TCounters & Counters)
{
  TCounters::iterator i = Counters.find(AKey);
  if (i != Counters.end())
  {
    if (Value > i->second)
    {
      i->second = Value;
    }
  }
  else
  {
    Counters[AKey] = Value;
  }
}
//---------------------------------------------------------------------------
void TUsage::SetCollect(bool Value)
{
  TGuard Guard(*FCriticalSection); nb::used(Guard);
  if (FCollect != Value)
  {
    FCollect = Value;
    if (!FCollect)
    {
      FPeriodCounters.clear();
      FLifetimeCounters.clear();
      FValues->Clear();
    }
    else
    {
      Default();
    }
  }
}
//---------------------------------------------------------------------------
UnicodeString TUsage::Serialize(const UnicodeString ADelimiter, const UnicodeString AFilter) const
{
  TGuard Guard(*FCriticalSection); nb::used(Guard);
  UnicodeString Result;

  UnicodeString FilterUpper = UpperCase(AFilter);
  for (int Index = 0; Index < FValues->Count; Index++)
  {
    Serialize(Result, FValues->GetName(Index), FValues->GetValueFromIndex(Index), ADelimiter, FilterUpper);
  }

  Serialize(Result, "Period", FPeriodCounters, ADelimiter, FilterUpper);
  Serialize(Result, "Lifetime", FLifetimeCounters, ADelimiter, FilterUpper);

  return Result;
}
//---------------------------------------------------------------------------
void TUsage::Serialize(
  UnicodeString & List, const UnicodeString AName, const TCounters & Counters,
  const UnicodeString ADelimiter, const UnicodeString AFilterUpper) const
{
  TCounters::const_iterator i = Counters.begin();
  while (i != Counters.end())
  {
    Serialize(List, AName + i->first, IntToStr(i->second), ADelimiter, AFilterUpper);
    i++;
  }
}
//---------------------------------------------------------------------------
void TUsage::Serialize(
  UnicodeString & AList, const UnicodeString AName, const UnicodeString AValue,
  const UnicodeString ADelimiter, const UnicodeString AFilterUpper) const
{
  if (AFilterUpper.IsEmpty() ||
      (UpperCase(AName).Pos(AFilterUpper) > 0) ||
      (UpperCase(AValue).Pos(AFilterUpper) > 0))
  {
    AddToList(AList, FORMAT("%s=%s", AName, AValue), ADelimiter);
  }
}
//---------------------------------------------------------------------------
int TUsage::CalculateCounterSize(int64_t Size)
{
  const int SizeCounterFactor = 10240;
  return (int)((Size <= 0) ? 0 : (Size < SizeCounterFactor ? 1 : Size / SizeCounterFactor));
}
