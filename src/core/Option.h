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
  /* __fastcall */ TOptions();

  bool __fastcall FindSwitch(const UnicodeString Switch);
  bool __fastcall FindSwitch(const UnicodeString Switch, UnicodeString & Value);
  bool __fastcall FindSwitch(const UnicodeString Switch, int & ParamsStart,
    int & ParamsCount);
  bool __fastcall FindSwitch(const UnicodeString Switch, TStrings * Params,
    int ParamsMax = -1);
  void __fastcall ParamsProcessed(int Position, int Count);
  UnicodeString __fastcall SwitchValue(const UnicodeString Switch, const UnicodeString Default = L"");
  bool __fastcall SwitchValue(const UnicodeString Switch, bool Default);
  bool __fastcall SwitchValue(const UnicodeString Switch, bool Default, bool DefaultOnNonExistence);
  bool __fastcall UnusedSwitch(UnicodeString & Switch);

#ifndef _MSC_VER
  __property int ParamCount = { read = FParamCount };
  __property UnicodeString Param[int Index] = { read = GetParam };
  __property bool Empty = { read = GetEmpty };
#else
  int __fastcall GetParamCount() { return FParamCount; }
  UnicodeString __fastcall GetParam(int Index);
  bool __fastcall GetEmpty();
#endif

protected:
  UnicodeString FSwitchMarks;
  UnicodeString FSwitchValueDelimiters;

  void __fastcall Add(UnicodeString Option);

  bool __fastcall FindSwitch(const UnicodeString Switch,
    UnicodeString & Value, int & ParamsStart, int & ParamsCount);

private:
  struct TOption
  {
    TOptionType Type;
    UnicodeString Name;
    UnicodeString Value;
    bool Used;
  };

  std::vector<TOption> FOptions;
  bool FNoMoreSwitches;
  int FParamCount;

#ifndef _MSC_VER
  UnicodeString __fastcall GetParam(int Index);
  bool __fastcall GetEmpty();
#endif
};
//---------------------------------------------------------------------------
#endif
