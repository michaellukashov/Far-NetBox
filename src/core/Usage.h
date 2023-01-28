
#pragma once

#include <rdestl/map.h>
#include "HierarchicalStorage.h"

class TConfiguration;

class TUsage : public TObject
{
public:
  explicit TUsage(TConfiguration * Configuration) noexcept;
  virtual ~TUsage() noexcept;

  void Set(const UnicodeString AKey, const UnicodeString AValue);
  void Set(const UnicodeString AKey, int32_t Value);
  void Set(const UnicodeString AKey, bool Value);
  int32_t Inc(const UnicodeString AKey, int32_t Increment = 1);
  void SetMax(const UnicodeString AKey, int32_t Value);
  UnicodeString Get(const UnicodeString AKey) const;

  void UpdateCurrentVersion();
  void Reset();

  void Default();
  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage) const;
  UnicodeString Serialize(const UnicodeString ADelimiter = "&", const UnicodeString AFilter = "") const;

  static int32_t CalculateCounterSize(int64_t Size);

  __property bool Collect = { read = FCollect, write = SetCollect };
  RWProperty<bool> Collect{nb::bind(&TUsage::GetCollect, this), nb::bind(&TUsage::SetCollect, this)};

private:
  using TCounters = nb::map_t<UnicodeString, int32_t>;
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
    const UnicodeString AName, TCounters & Counters);
  void Save(THierarchicalStorage * Storage,
    const UnicodeString AName, const TCounters & Counters) const;
  int32_t Inc(const UnicodeString AKey, TCounters & Counters, int32_t Increment);
  void SetMax(const UnicodeString AKey, int32_t Value, TCounters & Counters);
  void Serialize(
    UnicodeString& AList, const UnicodeString AName, const TCounters & Counters,
    const UnicodeString ADelimiter, const UnicodeString AFilterUpper) const;
  void Serialize(
    UnicodeString & List, const UnicodeString AName, const UnicodeString AValue,
    const UnicodeString ADelimiter, const UnicodeString AFilterUpper) const;
  void ResetLastExceptions();
  void ResetValue(const UnicodeString AKey);
};

extern const UnicodeString LastInternalExceptionCounter;
extern const UnicodeString LastUpdateExceptionCounter;

