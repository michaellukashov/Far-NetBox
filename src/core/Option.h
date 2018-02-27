
#pragma once

#include <Classes.hpp>
//---------------------------------------------------------------------------
enum TOptionType
{
  otParam,
  otSwitch
};
//---------------------------------------------------------------------------
#if 0
typedef void (__closure *TLogOptionEvent)(const UnicodeString LogStr);
#endif // #if 0
typedef nb::FastDelegate1<void, UnicodeString /*LogStr*/> TLogOptionEvent;
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TOptions : public TObject
{
public:
  TOptions();

  void Add(const UnicodeString Value);

  bool FindSwitch(const UnicodeString Switch);
  bool FindSwitch(const UnicodeString Switch, UnicodeString &Value);
  bool FindSwitch(const UnicodeString Switch, UnicodeString &Value, bool &ValueSet);
  bool FindSwitch(const UnicodeString Switch, intptr_t &ParamsStart,
    intptr_t &ParamsCount);
  bool FindSwitch(const UnicodeString Switch, TStrings *Params,
    intptr_t ParamsMax = -1);
  bool FindSwitchCaseSensitive(const UnicodeString Switch);
  bool FindSwitchCaseSensitive(const UnicodeString Switch, TStrings *Params,
    intptr_t ParamsMax = -1);
  void ParamsProcessed(intptr_t ParamsStart, intptr_t ParamsCount);
  UnicodeString SwitchValue(const UnicodeString Switch, const UnicodeString Default = L"");
  bool SwitchValue(const UnicodeString Switch, bool Default);
  bool SwitchValue(const UnicodeString Switch, bool Default, bool DefaultOnNonExistence);
  bool UnusedSwitch(UnicodeString &Switch) const;
  bool WasSwitchAdded(UnicodeString &Switch, wchar_t &SwitchMark) const;

  void LogOptions(TLogOptionEvent OnEnumOption);

  __property int ParamCount = { read = FParamCount };
  __property UnicodeString Param[int Index] = { read = GetParam };
  __property bool Empty = { read = GetEmpty };

  intptr_t GetParamCount() const { return FParamCount; }
  void Clear() { FOptions.resize(0); FNoMoreSwitches = false; FParamCount = 0; }
  UnicodeString GetSwitchMarks() const { return FSwitchMarks; }

protected:
  UnicodeString FSwitchMarks;
  UnicodeString FSwitchValueDelimiters;

  bool FindSwitch(const UnicodeString Switch,
    UnicodeString &Value, intptr_t &ParamsStart, intptr_t &ParamsCount, bool CaseSensitive, bool &ValueSet);
  bool DoFindSwitch(const UnicodeString Switch, TStrings *Params,
    intptr_t ParamsMax, bool CaseSensitive);

private:
  struct TOption : public TObject
  {
    TOption() : Type(otParam), ValueSet(false), Used(false), SwitchMark(0) {}
    UnicodeString Name;
    UnicodeString Value;
    TOptionType Type;
    bool ValueSet;
    bool Used;
    wchar_t SwitchMark;
  };

  typedef rde::vector<TOption> TOptionsVector;
  TOptionsVector FOptions;
  TOptionsVector FOriginalOptions;
  intptr_t FParamCount;
  bool FNoMoreSwitches;

public:
  UnicodeString GetParam(intptr_t AIndex);
  bool GetEmpty() const;
};
//---------------------------------------------------------------------------
