//---------------------------------------------------------------------------
#pragma once

#include <rdestl/map.h>
#include "HierarchicalStorage.h"
//---------------------------------------------------------------------------
class TConfiguration;
//---------------------------------------------------------------------------
class TUsage : public TObject
{
public:
  explicit TUsage(TConfiguration * Configuration) noexcept;
  virtual ~TUsage() noexcept;

  void Set(UnicodeString AKey, UnicodeString AValue);
  void Set(UnicodeString AKey, intptr_t Value);
  void Set(UnicodeString AKey, bool Value);
  void Inc(UnicodeString AKey, intptr_t Increment = 1);
  void SetMax(UnicodeString AKey, intptr_t Value);
  UnicodeString Get(UnicodeString AKey) const;

  void UpdateCurrentVersion();
  void Reset();

  void Default();
  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage) const;
  UnicodeString Serialize(UnicodeString ADelimiter = "&", UnicodeString AFilter = "") const;

  static intptr_t CalculateCounterSize(int64_t Size);

  __property bool Collect = { read = FCollect, write = SetCollect };
  RWProperty<bool> Collect{nb::bind(&TUsage::GetCollect, this), nb::bind(&TUsage::SetCollect, this)};

private:
  using TCounters = nb::map_t<UnicodeString, intptr_t>;
  std::unique_ptr<TCriticalSection> FCriticalSection;
  gsl::not_null<TConfiguration *> FConfiguration;
  TCounters FPeriodCounters;
  TCounters FLifetimeCounters;
  std::unique_ptr<TStringList> FValues;
  bool FCollect{true};

public:
  bool GetCollect() const { return FCollect; }
  void SetCollect(bool value);
  void UpdateLastReport();
  void Load(THierarchicalStorage * Storage,
    UnicodeString AName, TCounters & Counters);
  void Save(THierarchicalStorage * Storage,
    UnicodeString AName, const TCounters & Counters) const;
  void Inc(UnicodeString AKey, TCounters & Counters, intptr_t Increment);
  void SetMax(UnicodeString AKey, intptr_t Value, TCounters & Counters);
  void Serialize(
    UnicodeString& AList, UnicodeString AName, const TCounters & Counters,
    UnicodeString ADelimiter, UnicodeString AFilterUpper) const;
  void Serialize(
    UnicodeString & List, UnicodeString AName, UnicodeString AValue,
    UnicodeString ADelimiter, UnicodeString AFilterUpper) const;
  void ResetLastExceptions();
  void ResetValue(UnicodeString AKey);
};
//---------------------------------------------------------------------------
extern UnicodeString LastInternalExceptionCounter;
extern UnicodeString LastUpdateExceptionCounter;
//---------------------------------------------------------------------------
