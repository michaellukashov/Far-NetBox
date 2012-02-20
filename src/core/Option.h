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
    bool FindSwitch(const std::wstring Switch, std::wstring &Value);
    bool FindSwitch(const std::wstring Switch, int &ParamsStart,
                    int &ParamsCount);
    bool FindSwitch(const std::wstring Switch, nb::TStrings *Params,
                    int ParamsMax = -1);
    void ParamsProcessed(int Position, int Count);
    std::wstring SwitchValue(const std::wstring Switch, const std::wstring Default = L"");
    bool UnusedSwitch(std::wstring &Switch);

    int GetParamCount() { return FParamCount; }
    std::wstring GetParam(int Index);
    bool GetEmpty();

protected:
    std::wstring FSwitchMarks;
    std::wstring FSwitchValueDelimiters;

    void Add(const std::wstring Option);

    bool FindSwitch(const std::wstring Switch,
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
