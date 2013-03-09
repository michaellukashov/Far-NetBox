//---------------------------------------------------------------------------
#ifndef OptionH
#define OptionH

#include <vector>
//---------------------------------------------------------------------------
enum TOptionType { otParam, otSwitch };
//---------------------------------------------------------------------------
class TOptions
{
public:
  TOptions();
  void ParseParams(const UnicodeString & Params);

  bool FindSwitch(const UnicodeString & Switch);
  bool FindSwitch(const UnicodeString & Switch, UnicodeString & Value);
  bool FindSwitch(const UnicodeString & Switch, intptr_t & ParamsStart,
    intptr_t & ParamsCount);
  bool FindSwitch(const UnicodeString & Switch, TStrings * Params,
    intptr_t ParamsMax = -1);
  void ParamsProcessed(intptr_t Position, intptr_t Count);
  UnicodeString SwitchValue(const UnicodeString & Switch, const UnicodeString & Default = L"");
  bool SwitchValue(const UnicodeString & Switch, bool Default);
  bool SwitchValue(const UnicodeString & Switch, bool Default, bool DefaultOnNonExistence);
  bool UnusedSwitch(UnicodeString & Switch) const;

  intptr_t GetParamCount() const { return FParamCount; }
  UnicodeString GetParam(intptr_t Index);
  void Clear() { FOptions.resize(0); FNoMoreSwitches = false; FParamCount = 0; }
  bool GetEmpty() const;
  UnicodeString GetSwitchMarks() const { return FSwitchMarks; }

protected:
  UnicodeString FSwitchMarks;
  UnicodeString FSwitchValueDelimiters;

  void Add(const UnicodeString & Option);

  bool FindSwitch(const UnicodeString & Switch,
    UnicodeString & Value, intptr_t & ParamsStart, intptr_t & ParamsCount);

private:
  struct TOption
  {
    TOptionType Type;
    UnicodeString Name;
    UnicodeString Value;
    bool Used;
  };

  std::vector<TOption, custom_nballocator_t<TOption> > FOptions;
  bool FNoMoreSwitches;
  intptr_t FParamCount;
};
//---------------------------------------------------------------------------
#endif
