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

    bool __fastcall FindSwitch(const std::wstring Switch);
    bool __fastcall FindSwitch(const std::wstring Switch, std::wstring &Value);
    bool __fastcall FindSwitch(const std::wstring Switch, int &ParamsStart,
                    int &ParamsCount);
    bool __fastcall FindSwitch(const std::wstring Switch, System::TStrings *Params,
                    int ParamsMax = -1);
    void __fastcall ParamsProcessed(int Position, int Count);
    std::wstring __fastcall SwitchValue(const std::wstring Switch, const std::wstring Default = L"");
    bool __fastcall UnusedSwitch(std::wstring &Switch);

    int __fastcall GetParamCount() { return FParamCount; }
    std::wstring __fastcall GetParam(int Index);
    bool __fastcall GetEmpty();

protected:
    std::wstring FSwitchMarks;
    std::wstring FSwitchValueDelimiters;

    void __fastcall Add(const std::wstring Option);

    bool __fastcall FindSwitch(const std::wstring Switch,
                    std::wstring &Value, int &ParamsStart, int &ParamsCount);

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
