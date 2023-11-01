
#pragma once

#include <Classes.hpp>

enum TOptionType { otParam, otSwitch };

using TLogOptionEvent = nb::FastDelegate1<void, UnicodeString /*LogStr*/>;

class NB_CORE_EXPORT TOptions : public TObject
{
public:
  TOptions() noexcept;
  TOptions(const TOptions & Source);

  void Add(const UnicodeString Value);
  void Parse(const UnicodeString & CmdLine);

  bool FindSwitch(const UnicodeString Switch);
  bool FindSwitch(const UnicodeString Switch, UnicodeString & Value);
  bool FindSwitch(const UnicodeString Switch, UnicodeString & Value, bool & ValueSet);
  bool FindSwitch(const UnicodeString Switch, TStrings * Params,
    int32_t ParamsMax = -1);
  bool FindSwitchCaseSensitive(const UnicodeString Switch);
  bool FindSwitchCaseSensitive(const UnicodeString Switch, TStrings * Params,
    int32_t ParamsMax = -1);
  UnicodeString SwitchValue(const UnicodeString Switch, const UnicodeString Default = L"");
  bool SwitchValue(const UnicodeString Switch, bool Default);
  bool SwitchValue(const UnicodeString Switch, bool Default, bool DefaultOnNonExistence);
  bool UnusedSwitch(UnicodeString & Switch) const;
  bool WasSwitchAdded(UnicodeString & Switch, UnicodeString & Value, wchar_t & SwitchMark) const;
  UnicodeString ConsumeParam();

  void LogOptions(TLogOptionEvent OnEnumOption);

  __property int ParamCount = { read = FParamCount };
  __property UnicodeString Param[int Index] = { read = GetParam };
  __property bool Empty = { read = GetEmpty };

  int32_t GetParamCount() const { return FParamCount; }
  void Clear() { FOptions.resize(0); FNoMoreSwitches = false; FParamCount = 0; }
  UnicodeString GetSwitchMarks() const { return FSwitchMarks; }

protected:
  UnicodeString FSwitchMarks;
  UnicodeString FSwitchValueDelimiters;

  bool FindSwitch(const UnicodeString Switch,
    UnicodeString &Value, int32_t &ParamsStart, int32_t &ParamsCount, bool CaseSensitive, bool &ValueSet);
  bool DoFindSwitch(const UnicodeString Switch, TStrings *Params,
    int32_t ParamsMax, bool CaseInsensitive);
  void ParamsProcessed(int32_t Position, int32_t Count);

private:
  struct TOption : public TObject
  {
    TOption() noexcept : Type(otParam), ValueSet(false), Used(false), SwitchMark(0) {}
    TOptionType Type{otParam};
    UnicodeString Name;
    UnicodeString Value;
    bool ValueSet{false};
    bool Used{false};
    wchar_t SwitchMark{};
  };

  typedef nb::vector_t<TOption> TOptionsVector;
  mutable TOptionsVector FOptions;
  TOptionsVector FOriginalOptions;
  bool FNoMoreSwitches{false};
  int32_t FParamCount{0};

public:
  UnicodeString GetParam(int32_t AIndex) const;
  bool GetEmpty() const;
};

