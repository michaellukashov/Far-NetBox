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

  bool FindSwitch(const wstring Switch);
  bool FindSwitch(const wstring Switch, wstring & Value);
  bool FindSwitch(const wstring Switch, int & ParamsStart,
    int & ParamsCount);
  bool FindSwitch(const wstring Switch, TStrings * Params,
    int ParamsMax = -1);
  void ParamsProcessed(int Position, int Count);
  wstring SwitchValue(const wstring Switch, const wstring Default = "");
  bool UnusedSwitch(wstring & Switch);

  __property int ParamCount = { read = FParamCount };
  __property wstring Param[int Index] = { read = GetParam };
  __property bool Empty = { read = GetEmpty };

protected:
  wstring FSwitchMarks;
  wstring FSwitchValueDelimiters;

  void Add(wstring Option);

  bool FindSwitch(const wstring Switch,
    wstring & Value, int & ParamsStart, int & ParamsCount);

private:
  struct TOption
  {
    TOptionType Type;
    wstring Name;
    wstring Value;
    bool Used;
  };

  std::vector<TOption> FOptions;
  bool FNoMoreSwitches;
  int FParamCount;

  wstring GetParam(int Index);
  bool GetEmpty();
};
//---------------------------------------------------------------------------
#endif
