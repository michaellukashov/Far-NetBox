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
  explicit TUsage(TConfiguration * Configuration);
  virtual ~TUsage();

  void Set(const UnicodeString AKey, const UnicodeString AValue);
  void Set(const UnicodeString AKey, intptr_t Value);
  void Set(const UnicodeString AKey, bool Value);
  void Inc(const UnicodeString AKey, intptr_t Increment = 1);
  void SetMax(const UnicodeString AKey, intptr_t Value);
  UnicodeString Get(const UnicodeString AKey) const;

  void UpdateCurrentVersion();
  void Reset();

  void Default();
  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage) const;
  UnicodeString Serialize(const UnicodeString ADelimiter = L"&", const UnicodeString AFilter = L"") const;

  static int CalculateCounterSize(int64_t Size);

  __property bool Collect = { read = FCollect, write = SetCollect };

private:
  typedef rde::map<UnicodeString, intptr_t> TCounters;
  std::unique_ptr<TCriticalSection> FCriticalSection;
  TConfiguration * FConfiguration{nullptr};
  TCounters FPeriodCounters;
  TCounters FLifetimeCounters;
  std::unique_ptr<TStringList> FValues;
  bool FCollect{true};

  void SetCollect(bool value);
  void UpdateLastReport();
  void Load(THierarchicalStorage * Storage,
    const UnicodeString AName, TCounters & Counters);
  void Save(THierarchicalStorage * Storage,
    const UnicodeString AName, const TCounters & Counters) const;
  void Inc(const UnicodeString AKey, TCounters & Counters, intptr_t Increment);
  void SetMax(const UnicodeString AKey, intptr_t Value, TCounters & Counters);
  void Serialize(
    UnicodeString& AList, const UnicodeString AName, const TCounters & Counters,
    const UnicodeString ADelimiter, const UnicodeString AFilterUpper) const;
  void Serialize(
    UnicodeString & List, const UnicodeString AName, const UnicodeString AValue,
    const UnicodeString ADelimiter, const UnicodeString AFilterUpper) const;
  void ResetLastExceptions();
  void ResetValue(const UnicodeString AKey);
};
//---------------------------------------------------------------------------
extern const UnicodeString LastInternalExceptionCounter;
extern const UnicodeString LastUpdateExceptionCounter;
//---------------------------------------------------------------------------
