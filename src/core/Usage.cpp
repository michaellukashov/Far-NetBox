
#include <vcl.h>
// #pragma hdrstop

#include <Configuration.h>
#include <CoreMain.h>
#include <Common.h>
#include <Usage.h>
#include <FileInfo.h>

// #pragma package(smart_init)

#if defined(__BORLANDC__)
const UnicodeString LastInternalExceptionCounter("LastInternalException2");
const UnicodeString LastUpdateExceptionCounter("LastUpdateException");
#endif // defined(__BORLANDC__)

TUsage::TUsage(TConfiguration * Configuration) noexcept :
  FCriticalSection(std::make_unique<TCriticalSection>()),
  FConfiguration(Configuration),
  FValues(std::make_unique<TStringList>())
{
  FValues->SetDelimiter(L'&');
  FValues->SetStrictDelimiter(true);
  FCollect = true;
  Default();
}

TUsage::~TUsage() noexcept
{
#if defined(__BORLANDC__)
  delete FValues;
  delete FCriticalSection;
#endif // defined(__BORLANDC__)
}

void TUsage::Default()
{
  const TGuard Guard(*FCriticalSection);
  FPeriodCounters.clear();
  FLifetimeCounters.clear();
  FValues->Clear();

  if (FCollect) // optimization
  {
    Set("FirstUse", StandardTimestamp());
    Set("FirstVersion", IntToStr(FConfiguration->CompoundVersion));
    UpdateLastReport();
  }
}

void TUsage::Load(THierarchicalStorage * Storage)
{
  const TGuard Guard(*FCriticalSection);
  Default();

  if (Storage->OpenSubKey("Values", false))
  {
    std::unique_ptr<TStrings> Names = std::make_unique<TStringList>();
    try__finally
    {
      Storage->GetValueNames(Names.get());
      for (int32_t Index = 0; Index < Names->Count; Index++)
      {
        UnicodeString Name = Names->GetString(Index);
        Set(Name, Storage->ReadString(Name, ""));
      }
      Storage->CloseSubKey();
    }
    __finally__removed
    {
#if defined(__BORLANDC__)
      delete Names;
#endif // defined(__BORLANDC__)
    } end_try__finally
  }

  Load(Storage, "PeriodCounters", FPeriodCounters);
  Load(Storage, "LifetimeCounters", FLifetimeCounters);
}

void TUsage::Load(THierarchicalStorage * Storage,
  const UnicodeString & AName, TCounters & Counters)
{
  if (Storage->OpenSubKey(AName, false))
  {
    std::unique_ptr<TStrings> Names = std::make_unique<TStringList>();
    try__finally
    {
      Storage->GetValueNames(Names.get());
      for (int32_t Index = 0; Index < Names->Count; Index++)
      {
        UnicodeString Name = Names->GetString(Index);
        Counters[Name] = Storage->ReadInteger(Name, 0);
      }
      Storage->CloseSubKey();
    }
    __finally__removed
    {
#if defined(__BORLANDC__)
      delete Names;
#endif // defined(__BORLANDC__)
    } end_try__finally
  }
}

void TUsage::Save(THierarchicalStorage * Storage) const
{
  const TGuard Guard(*FCriticalSection);
  if (Storage->OpenSubKey("Values", true))
  {
    Storage->ClearValues();
    Storage->WriteValues(FValues.get(), true);
    Storage->CloseSubKey();
  }

  Save(Storage, "PeriodCounters", FPeriodCounters);
  Save(Storage, "LifetimeCounters", FLifetimeCounters);
}

void TUsage::Save(THierarchicalStorage * Storage,
  const UnicodeString & AName, const TCounters & Counters) const
{
  if (Storage->OpenSubKey(AName, true))
  {
    Storage->ClearValues();
    TCounters::const_iterator i = Counters.begin();
    while (i != Counters.end())
    {
      Storage->WriteInteger(i->first, nb::ToInt32(i->second));
      ++i;
    }
    Storage->CloseSubKey();
  }
}

void TUsage::Set(const UnicodeString & AKey, const UnicodeString & AValue)
{
  if (FCollect)
  {
    const TGuard Guard(*FCriticalSection);
    FValues->SetValue(AKey, AValue);
  }
}

void TUsage::Set(const UnicodeString & AKey, int32_t Value)
{
  Set(AKey, IntToStr(Value));
}

void TUsage::Set(const UnicodeString & AKey, bool Value)
{
  Set(AKey, int32_t(Value ? 1 : 0));
}

UnicodeString TUsage::Get(const UnicodeString & AKey) const
{
  const TGuard Guard(*FCriticalSection);
  UnicodeString Result = FValues->GetValue(AKey);
  Result.Unique();
  return Result;
}

void TUsage::UpdateLastReport()
{
  Set("LastReport", StandardTimestamp());
}

void TUsage::Reset()
{
  const TGuard Guard(*FCriticalSection);
  UpdateLastReport();
  FPeriodCounters.clear();
  ResetLastExceptions();
}

void TUsage::UpdateCurrentVersion()
{
  const TGuard Guard(*FCriticalSection);
  int32_t CompoundVersion = FConfiguration->CompoundVersion;
  DebugAssert(ZeroBuildNumber(CompoundVersion) == CompoundVersion);
  // ZeroBuildNumber for compatibility with versions that stored build number into the compound version
  int32_t PrevVersion = ZeroBuildNumber(StrToIntDef(Get("CurrentVersion"), 0));
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

void TUsage::ResetValue(const UnicodeString & AKey)
{
  int32_t Index = FValues->IndexOfName(AKey);
  if (Index >= 0)
  {
    FValues->Delete(Index);
  }
}

void TUsage::ResetLastExceptions()
{
  const TGuard Guard(*FCriticalSection);
  ResetValue(LastInternalExceptionCounter);
  ResetValue(LastUpdateExceptionCounter);
}

int32_t TUsage::Inc(const UnicodeString & AKey, int32_t Increment)
{
  int Result;
  if (FCollect)
  {
    const TGuard Guard(*FCriticalSection);
    Inc(AKey, FPeriodCounters, Increment);
    Result = Inc(AKey, FLifetimeCounters, Increment);
  }
  else
  {
    Result = -1;
  }
  return Result;
}

int32_t TUsage::Inc(const UnicodeString & AKey, TCounters & Counters, int32_t Increment)
{
  int32_t Result;
  TCounters::iterator i = Counters.find(AKey);
  if (i != Counters.end())
  {
    i->second += Increment;
    Result = i->second;
  }
  else
  {
    Counters[AKey] = Increment;
    Result = Increment;
  }
  return Result;
}

void TUsage::SetMax(const UnicodeString & AKey, int32_t Value)
{
  if (FCollect)
  {
    const TGuard Guard(*FCriticalSection);
    SetMax(AKey, Value, FPeriodCounters);
    SetMax(AKey, Value, FLifetimeCounters);
  }
}

void TUsage::SetMax(const UnicodeString & AKey, int32_t Value,
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

void TUsage::SetCollect(bool Value)
{
  const TGuard Guard(*FCriticalSection);
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

UnicodeString TUsage::Serialize(const UnicodeString & ADelimiter, const UnicodeString & AFilter) const
{
  const TGuard Guard(*FCriticalSection);
  UnicodeString Result;

  const UnicodeString FilterUpper = UpperCase(AFilter);
  for (int32_t Index = 0; Index < FValues->Count; Index++)
  {
    Serialize(Result, FValues->GetName(Index), FValues->GetValueFromIndex(Index), ADelimiter, FilterUpper);
  }

  Serialize(Result, "Period", FPeriodCounters, ADelimiter, FilterUpper);
  Serialize(Result, "Lifetime", FLifetimeCounters, ADelimiter, FilterUpper);

  return Result;
}

void TUsage::Serialize(
  UnicodeString & List, const UnicodeString & AName, const TCounters & Counters,
  const UnicodeString & ADelimiter, const UnicodeString & AFilterUpper) const
{
  TCounters::const_iterator i = Counters.begin();
  while (i != Counters.end())
  {
    Serialize(List, AName + i->first, IntToStr(i->second), ADelimiter, AFilterUpper);
    ++i;
  }
}

void TUsage::Serialize(
  UnicodeString & AList, const UnicodeString & AName, const UnicodeString & AValue,
  const UnicodeString & ADelimiter, const UnicodeString & AFilterUpper) const
{
  if (AFilterUpper.IsEmpty() ||
      (UpperCase(AName).Pos(AFilterUpper) > 0) ||
      (UpperCase(AValue).Pos(AFilterUpper) > 0))
  {
    AddToList(AList, FORMAT("%s=%s", AName, AValue), ADelimiter);
  }
}

int32_t TUsage::CalculateCounterSize(int64_t Size)
{
  constexpr int32_t SizeCounterFactor = 10 * 1024;
  return (int32_t)((Size <= 0) ? 0 : (Size < SizeCounterFactor ? 1 : Size / SizeCounterFactor));
}
