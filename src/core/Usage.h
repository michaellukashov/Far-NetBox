//---------------------------------------------------------------------------
#pragma once

#include <rdestl/map.h>
#include "HierarchicalStorage.h"
//---------------------------------------------------------------------------
class TConfiguration;
//---------------------------------------------------------------------------
class TUsage
{
public:
  TUsage(TConfiguration * Configuration);
  virtual ~TUsage();

  void Set(const UnicodeString & Key, const UnicodeString & Value);
  void Set(const UnicodeString & Key, int Value);
  void Set(const UnicodeString & Key, bool Value);
  void Inc(const UnicodeString & Key, int Increment = 1);
  void SetMax(const UnicodeString & Key, int Value);
  UnicodeString Get(const UnicodeString & Key);

  void UpdateCurrentVersion();
  void Reset();

  void Default();
  void Load(THierarchicalStorage * Storage);
  void Save(THierarchicalStorage * Storage) const;
  UnicodeString Serialize(const UnicodeString & Delimiter = L"&", const UnicodeString & Filter = L"") const;

  static int CalculateCounterSize(int64_t Size);

  __property bool Collect = { read = FCollect, write = SetCollect };

private:
  typedef rde::map<UnicodeString, int> TCounters;
  TCriticalSection * FCriticalSection;
  TConfiguration * FConfiguration;
  TCounters FPeriodCounters;
  TCounters FLifetimeCounters;
  TStringList * FValues;
  bool FCollect;

  void SetCollect(bool value);
  void UpdateLastReport();
  void Load(THierarchicalStorage * Storage,
    const UnicodeString & Name, TCounters & Counters);
  void Save(THierarchicalStorage * Storage,
    const UnicodeString & Name, const TCounters & Counters) const;
  void Inc(const UnicodeString & Key, TCounters & Counters, int Increment);
  void SetMax(const UnicodeString & Key, int Value, TCounters & Counters);
  void Serialize(
    UnicodeString& List, const UnicodeString & Name, const TCounters & Counters,
    const UnicodeString & Delimiter, const UnicodeString & FilterUpper) const;
  void Serialize(
    UnicodeString & List, const UnicodeString & Name, const UnicodeString & Value,
    const UnicodeString & Delimiter, const UnicodeString & FilterUpper) const;
  void ResetLastExceptions();
  void ResetValue(const UnicodeString & Key);
};
//---------------------------------------------------------------------------
extern const UnicodeString LastInternalExceptionCounter;
extern const UnicodeString LastUpdateExceptionCounter;
//---------------------------------------------------------------------------
