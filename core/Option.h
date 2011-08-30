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

  bool FindSwitch(const std::wstring Switch);
  bool FindSwitch(const std::wstring Switch, std::wstring & Value);
  bool FindSwitch(const std::wstring Switch, int & ParamsStart,
    int & ParamsCount);
  bool FindSwitch(const std::wstring Switch, TStrings * Params,
    int ParamsMax = -1);
  void ParamsProcessed(int Position, int Count);
  std::wstring SwitchValue(const std::wstring Switch, const std::wstring Default = L"");
  bool UnusedSwitch(std::wstring & Switch);

  // __property int ParamCount = { read = FParamCount };
  int GetParamCount() { return FParamCount; }
  // __property std::wstring Param[int Index] = { read = GetParam };
  std::wstring GetParam(int Index);
  // __property bool Empty = { read = GetEmpty };
  bool GetEmpty();

protected:
  std::wstring FSwitchMarks;
  std::wstring FSwitchValueDelimiters;

  void Add(std::wstring Option);

  bool FindSwitch(const std::wstring Switch,
    std::wstring & Value, int & ParamsStart, int & ParamsCount);

private:
  struct TOption
  {
    TOptionType Type;
    std::wstring Name;
    std::wstring Value;
    bool Used;
  };

  std::vector<TOption> FOptions;
  bool FNoMoreSwitches;
  int FParamCount;
};
//---------------------------------------------------------------------------
#endif
