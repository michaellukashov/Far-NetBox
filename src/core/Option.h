
#pragma once

#include <Classes.hpp>

enum TOptionType
{
  otParam,
  otSwitch
};

#if 0
typedef void (__closure *TLogOptionEvent)(UnicodeString LogStr);
#endif // #if 0
typedef nb::FastDelegate1<void, UnicodeString /*LogStr*/> TLogOptionEvent;

class NB_CORE_EXPORT TOptions : public TObject
{
public:
  TOptions();

  void Add(UnicodeString Value);

  // void ParseParams(UnicodeString Params);

  bool FindSwitch(UnicodeString Switch);
  bool FindSwitch(UnicodeString Switch, UnicodeString & Value);
  bool FindSwitch(UnicodeString Switch, UnicodeString & Value, bool & ValueSet);
  bool FindSwitch(UnicodeString Switch, intptr_t & ParamsStart,
    intptr_t & ParamsCount);
  bool FindSwitch(UnicodeString Switch, TStrings * Params,
    intptr_t ParamsMax = -1);
  bool FindSwitchCaseSensitive(UnicodeString Switch);
  bool FindSwitchCaseSensitive(UnicodeString Switch, TStrings * Params,
    int ParamsMax = -1);
  void ParamsProcessed(intptr_t ParamsStart, intptr_t ParamsCount);
  UnicodeString SwitchValue(UnicodeString Switch, UnicodeString Default = L"");
  bool SwitchValue(UnicodeString Switch, bool Default);
  bool SwitchValue(UnicodeString Switch, bool Default, bool DefaultOnNonExistence);
  bool UnusedSwitch(UnicodeString & Switch) const;
  bool WasSwitchAdded(UnicodeString & Switch, wchar_t & SwitchMark) const;

  void LogOptions(TLogOptionEvent OnLogOption);
#if 0
 __property int ParamCount = { read = FParamCount };
  __property UnicodeString Param[int Index] = { read = GetParam };
  __property bool Empty = { read = GetEmpty };
#endif // #if 0

  intptr_t GetParamCount() const { return FParamCount; }
  void Clear() { FOptions.resize(0); FNoMoreSwitches = false; FParamCount = 0; }
  UnicodeString GetSwitchMarks() const { return FSwitchMarks; }

protected:
  UnicodeString FSwitchMarks;
  UnicodeString FSwitchValueDelimiters;

  bool FindSwitch(UnicodeString Switch,
    UnicodeString & Value, intptr_t & ParamsStart, intptr_t & ParamsCount, bool CaseSensitive, bool & ValueSet);
  bool DoFindSwitch(UnicodeString Switch, TStrings * Params,
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

