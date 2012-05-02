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

    bool __fastcall FindSwitch(const UnicodeString Switch);
    bool __fastcall FindSwitch(const UnicodeString Switch, UnicodeString &Value);
    bool __fastcall FindSwitch(const UnicodeString Switch, int &ParamsStart,
                    int &ParamsCount);
    bool __fastcall FindSwitch(const UnicodeString Switch, TStrings *Params,
                    int ParamsMax = -1);
    void __fastcall ParamsProcessed(int Position, int Count);
    UnicodeString __fastcall SwitchValue(const UnicodeString Switch, const UnicodeString Default = L"");
    bool __fastcall UnusedSwitch(UnicodeString &Switch);

    int __fastcall GetParamCount() { return FParamCount; }
    UnicodeString __fastcall GetParam(int Index);
    bool __fastcall GetEmpty();

protected:
    UnicodeString FSwitchMarks;
    UnicodeString FSwitchValueDelimiters;

    void __fastcall Add(const UnicodeString Option);

    bool __fastcall FindSwitch(const UnicodeString Switch,
                    UnicodeString &Value, int &ParamsStart, int &ParamsCount);

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
};
//---------------------------------------------------------------------------
#endif
