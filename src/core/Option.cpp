//---------------------------------------------------------------------------
#include "stdafx.h"
#include "Common.h"
#include "Option.h"
//---------------------------------------------------------------------------
TOptions::TOptions()
{
    FSwitchMarks = L"-/";
    FSwitchValueDelimiters = L":=";
    FNoMoreSwitches = false;
    FParamCount = 0;
}
//---------------------------------------------------------------------------
void __fastcall TOptions::Add(const UnicodeString Value)
{
    if (!FNoMoreSwitches &&
            (Value.Length() == 2) &&
            (Value[0] == Value[1]) &&
            (FSwitchMarks.find_first_of(Value[0]) >= 0))
    {
        FNoMoreSwitches = true;
    }
    else
    {
        bool Switch = false;
        size_t Index = 0; // shut up
        if (!FNoMoreSwitches &&
                (Value.Length() >= 2) &&
                (FSwitchMarks.find_first_of(Value[0]) >= 0))
        {
            Index = 2;
            Switch = true;
            while (Switch && (Index < Value.Length()))
            {
                if (::IsDelimiter(Value, FSwitchValueDelimiters, Index))
                {
                    break;
                }
                // this is to treat /home/martin as parameter, not as switch
                else if ((Value[Index] != '?') && ((::UpCase(Value[Index]) < 'A') || (UpCase(Value[Index]) > 'Z')))
                {
                    Switch = false;
                    break;
                }
                ++Index;
            }
        }

        if (Switch)
        {
            TOption Option;
            Option.Type = otSwitch;
            Option.Name = Value.SubString(2, Index - 2);
            Option.Value = Value.SubString(Index + 1, Value.Length());
            Option.Used = false;
            FOptions.push_back(Option);
        }
        else
        {
            TOption Option;
            Option.Type = otParam;
            Option.Value = Value;
            Option.Used = false;
            FOptions.push_back(Option);
            ++FParamCount;
        }
    }
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TOptions::GetParam(int Index)
{
    assert((Index >= 1) && (Index <= FParamCount));

    UnicodeString Result;
    size_t I = 0;
    while ((I < FOptions.Length()) && (Index > 0))
    {
        if (FOptions[I].Type == otParam)
        {
            --Index;
            if (Index == 0)
            {
                Result = FOptions[I].Value;
                FOptions[I].Used = true;
            }
        }
        ++I;
    }

    return Result;
}
//---------------------------------------------------------------------------
bool __fastcall TOptions::GetEmpty()
{
    return FOptions.IsEmpty();
}
//---------------------------------------------------------------------------
bool __fastcall TOptions::FindSwitch(const UnicodeString Switch,
                          UnicodeString &Value, int &ParamsStart, int &ParamsCount)
{
    ParamsStart = 0;
    int Index = 0;
    bool Found = false;
    while ((Index < static_cast<int>(FOptions.Length())) && !Found)
    {
        UnicodeString S;
        if (FOptions[Index].Type == otParam)
        {
            ParamsStart++;
        }
        else if (FOptions[Index].Type == otSwitch)
        {
            if (AnsiSameText(FOptions[Index].Name, Switch))
            {
                Found = true;
                Value = FOptions[Index].Value;
                FOptions[Index].Used = true;
            }
        }
        Index++;
    }

    ParamsCount = 0;
    if (Found)
    {
        ParamsStart++;
        while ((Index + ParamsCount < static_cast<int>(FOptions.Length())) &&
                (FOptions[Index + ParamsCount].Type == otParam))
        {
            ParamsCount++;
        }
    }
    else
    {
        ParamsStart = 0;
    }

    return Found;
}
//---------------------------------------------------------------------------
bool __fastcall TOptions::FindSwitch(const UnicodeString Switch, UnicodeString &Value)
{
    int ParamsStart;
    int ParamsCount;
    return FindSwitch(Switch, Value, ParamsStart, ParamsCount);
}
//---------------------------------------------------------------------------
bool __fastcall TOptions::FindSwitch(const UnicodeString Switch)
{
    UnicodeString Value;
    int ParamsStart;
    int ParamsCount;
    return FindSwitch(Switch, Value, ParamsStart, ParamsCount);
}
//---------------------------------------------------------------------------
bool __fastcall TOptions::FindSwitch(const UnicodeString Switch,
                          TStrings *Params, int ParamsMax)
{
    UnicodeString Value;
    int ParamsStart;
    int ParamsCount;
    bool Result = FindSwitch(Switch, Value, ParamsStart, ParamsCount);
    if (Result)
    {
        if ((ParamsMax >= 0) && (ParamsCount > ParamsMax))
        {
            ParamsCount = ParamsMax;
        }

        int Index = 0;
        while (Index < ParamsCount)
        {
            Params->Add(GetParam(ParamsStart + Index));
            Index++;
        }
        ParamsProcessed(ParamsStart, ParamsCount);
    }
    return Result;
}
//---------------------------------------------------------------------------
UnicodeString __fastcall TOptions::SwitchValue(const UnicodeString Switch,
                                   const UnicodeString Default)
{
    UnicodeString Value;
    FindSwitch(Switch, Value);
    if (Value.IsEmpty())
    {
        Value = Default;
    }
    return Value;
}
//---------------------------------------------------------------------------
bool __fastcall TOptions::UnusedSwitch(UnicodeString &Switch)
{
    bool Result = false;
    size_t Index = 0;
    while (!Result && (Index < FOptions.Length()))
    {
        if ((FOptions[Index].Type == otSwitch) &&
                !FOptions[Index].Used)
        {
            Switch = FOptions[Index].Name;
            Result = true;
        }
        ++Index;
    }

    return Result;
}
//---------------------------------------------------------------------------
void __fastcall TOptions::ParamsProcessed(int ParamsStart, int ParamsCount)
{
    if (ParamsCount > 0)
    {
        assert((ParamsStart >= 0) && ((ParamsStart - ParamsCount + 1) <= FParamCount));

        size_t Index = 0;
        while ((Index < FOptions.Length()) && (ParamsStart > 0))
        {
            UnicodeString S;
            if (FOptions[Index].Type == otParam)
            {
                --ParamsStart;

                if (ParamsStart == 0)
                {
                    while (ParamsCount > 0)
                    {
                        assert(Index < FOptions.Length());
                        assert(FOptions[Index].Type == otParam);
                        FOptions.Delete(FOptions.begin() + Index);
                        --FParamCount;
                        --ParamsCount;
                    }
                }
            }
            Index++;
        }
    }
}
