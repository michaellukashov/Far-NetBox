//---------------------------------------------------------------------------
#include <Common.h>
#include "Option.h"
//---------------------------------------------------------------------------
TOptions::TOptions()
{
  FSwitchMarks = "-/";
  FSwitchValueDelimiters = ":=";
  FNoMoreSwitches = false;
  FParamCount = 0;
}
//---------------------------------------------------------------------------
void TOptions::Add(wstring Value)
{
  if (!FNoMoreSwitches &&
      (Value.Length() == 2) &&
      (Value[1] == Value[2]) &&
      (FSwitchMarks.Pos(Value[1]) > 0))
  {
    FNoMoreSwitches = true;
  }
  else
  {
    bool Switch = false;
    int Index = 0; // shut up
    if (!FNoMoreSwitches &&
        (Value.Length() >= 2) &&
        (FSwitchMarks.Pos(Value[1]) > 0))
    {
      Index = 2;
      Switch = true;
      while (Switch && (Index <= Value.Length()))
      {
        if (Value.IsDelimiter(FSwitchValueDelimiters, Index))
        {
          break;
        }
        // this is to treat /home/martin as parameter, not as switch
        else if ((Value[Index] != '?') && ((UpCase(Value[Index]) < 'A') || (UpCase(Value[Index]) > 'Z')))
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
wstring TOptions::GetParam(int Index)
{
  assert((Index >= 1) && (Index <= FParamCount));

  wstring Result;
  size_t I = 0;
  while ((I < FOptions.size()) && (Index > 0))
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
bool TOptions::GetEmpty()
{
  return FOptions.empty();
}
//---------------------------------------------------------------------------
bool TOptions::FindSwitch(const wstring Switch,
  wstring & Value, int & ParamsStart, int & ParamsCount)
{
  ParamsStart = 0;
  int Index = 0;
  bool Found = false;
  while ((Index < int(FOptions.size())) && !Found)
  {
    wstring S;
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
    while ((Index + ParamsCount < int(FOptions.size())) &&
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
bool TOptions::FindSwitch(const wstring Switch, wstring & Value)
{
  int ParamsStart;
  int ParamsCount;
  return FindSwitch(Switch, Value, ParamsStart, ParamsCount);
}
//---------------------------------------------------------------------------
bool TOptions::FindSwitch(const wstring Switch)
{
  wstring Value;
  int ParamsStart;
  int ParamsCount;
  return FindSwitch(Switch, Value, ParamsStart, ParamsCount);
}
//---------------------------------------------------------------------------
bool TOptions::FindSwitch(const wstring Switch,
  TStrings * Params, int ParamsMax)
{
  wstring Value;
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
      Params->Add(Param[ParamsStart + Index]);
      Index++;
    }
    ParamsProcessed(ParamsStart, ParamsCount);
  }
  return Result;
}
//---------------------------------------------------------------------------
wstring TOptions::SwitchValue(const wstring Switch,
  const wstring Default)
{
  wstring Value;
  FindSwitch(Switch, Value);
  if (Value.empty())
  {
    Value = Default;
  }
  return Value;
}
//---------------------------------------------------------------------------
bool TOptions::UnusedSwitch(wstring & Switch)
{
  bool Result = false;
  size_t Index = 0;
  while (!Result && (Index < FOptions.size()))
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
void TOptions::ParamsProcessed(int ParamsStart, int ParamsCount)
{
  if (ParamsCount > 0)
  {
    assert((ParamsStart >= 0) && ((ParamsStart - ParamsCount + 1) <= FParamCount));

    size_t Index = 0;
    while ((Index < FOptions.size()) && (ParamsStart > 0))
    {
      wstring S;
      if (FOptions[Index].Type == otParam)
      {
        --ParamsStart;

        if (ParamsStart == 0)
        {
          while (ParamsCount > 0)
          {
            assert(Index < FOptions.size());
            assert(FOptions[Index].Type == otParam);
            FOptions.erase(FOptions.begin() + Index);
            --FParamCount;
            --ParamsCount;
          }
        }
      }
      Index++;
    }
  }
}
