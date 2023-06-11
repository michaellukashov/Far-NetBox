
#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include "Option.h"
#include "TextsCore.h"
#include "System.StrUtils.hpp"

__removed #pragma package(smart_init)

const wchar_t ArrayValueDelimiter = L'[';
const wchar_t ArrayValueEnd = L']';

TOptions::TOptions() noexcept
{
  FSwitchValueDelimiters = UnicodeString(L"=:") + ArrayValueDelimiter;
}

TOptions::TOptions(const TOptions & Source)
{
  FSwitchMarks = Source.FSwitchMarks;
  FSwitchValueDelimiters = Source.FSwitchValueDelimiters;
  FOptions = Source.FOptions;
  FOriginalOptions = Source.FOriginalOptions;
  FNoMoreSwitches = Source.FNoMoreSwitches;
  FParamCount = Source.FParamCount;
}

void TOptions::Parse(const UnicodeString & CmdLine)
{
  UnicodeString ACmdLine = CmdLine;
  UnicodeString Param;
  while (CutToken(ACmdLine, Param))
  {
    Add(Param);
  }
}

void TOptions::Add(const UnicodeString Value)
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
    int32_t Index = 0; // shut up
    wchar_t SwitchMark = L'\0';
    wchar_t ValueDelimiter = L'\0';
    if (!FNoMoreSwitches &&
        (Value.Length() >= 2) &&
        (FSwitchMarks.Pos(Value[1]) > 0))
    {
      Index = 2;
      Switch = true;
      SwitchMark = Value[1];
      while (Switch && (Index <= Value.Length()))
      {
        if (Value.IsDelimiter(FSwitchValueDelimiters, Index))
        {
          ValueDelimiter = Value[Index];
          break;
        }
        // This is to treat /home/martin as parameter, not as switch.
        else if ((Value[Index] == L'?') ||
                 IsLetter(Value[Index]) ||
                 ((Value[Index] == L'-') && (SwitchMark == L'-') && (Value[2] == L'-'))) // allow --puttygen-switches
        {
          // noop
        }
        else
        {
          Switch = false;
          break;
        }
        ++Index;
      }
    }

    TOption Option;

    if (Switch)
    {
      Option.Type = otSwitch;
      Option.Name = Value.SubString(2, Index - 2);
      Option.Value = Value.SubString(Index + 1, Value.Length());
      if ((ValueDelimiter == ArrayValueDelimiter) && EndsStr(ArrayValueEnd, Option.Value))
      {
        Option.Value.SetLength(Option.Value.Length() - 1);
      }
      Option.ValueSet = (Index <= Value.Length());
    }
    else
    {
      Option.Type = otParam;
      Option.Value = Value;
      Option.ValueSet = false; // unused
      ++FParamCount;
    }

    Option.Used = false;
    Option.SwitchMark = SwitchMark;

    FOptions.push_back(Option);
  }

  FOriginalOptions = FOptions;
}

UnicodeString TOptions::GetParam(int32_t AIndex) const
{
  DebugAssert((AIndex >= 1) && (AIndex <= FParamCount));

  UnicodeString Result;
  size_t Idx = 0;
  while ((Idx < FOptions.size()) && (AIndex > 0))
  {
    if (FOptions[Idx].Type == otParam)
    {
      --AIndex;
      if (AIndex == 0)
      {
        Result = FOptions[Idx].Value;
        FOptions[Idx].Used = true;
      }
    }
    ++Idx;
  }

  return Result;
}

UnicodeString TOptions::ConsumeParam()
{
  UnicodeString Result = Param[1];
  ParamsProcessed(1, 1);
  return Result;
}

bool TOptions::GetEmpty() const
{
  return FOptions.empty();
}

bool TOptions::FindSwitch(const UnicodeString Switch,
  UnicodeString &Value, int32_t &ParamsStart, int32_t &ParamsCount, bool CaseSensitive, bool &ValueSet)
{
  ParamsStart = 0;
  ValueSet = false;
  int32_t Index = 0;
  bool Found = false;
  while ((Index < nb::ToIntPtr(FOptions.size())) && !Found)
  {
    if (FOptions[Index].Type == otParam)
    {
      ParamsStart++;
    }
    else if (FOptions[Index].Type == otSwitch)
    {
      if ((!CaseSensitive && ::SameText(FOptions[Index].Name, Switch)) ||
          (CaseSensitive && ::SameText(FOptions[Index].Name, Switch)))
      {
        Found = true;
        Value = FOptions[Index].Value;
        ValueSet = FOptions[Index].ValueSet;
        FOptions[Index].Used = true;
      }
    }
    ++Index;
  }

  ParamsCount = 0;
  if (Found)
  {
    ParamsStart++;
    while ((Index + ParamsCount < nb::ToIntPtr(FOptions.size())) &&
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

bool TOptions::FindSwitch(const UnicodeString Switch, UnicodeString &Value)
{
  bool ValueSet;
  return FindSwitch(Switch, Value, ValueSet);
}

bool TOptions::FindSwitch(const UnicodeString Switch, UnicodeString &Value, bool &ValueSet)
{
  int32_t ParamsStart;
  int32_t ParamsCount;
  return FindSwitch(Switch, Value, ParamsStart, ParamsCount, false, ValueSet);
}

bool TOptions::FindSwitch(const UnicodeString Switch)
{
  UnicodeString Value;
  int32_t ParamsStart;
  int32_t ParamsCount;
  bool ValueSet;
  return FindSwitch(Switch, Value, ParamsStart, ParamsCount, false, ValueSet);
}

bool TOptions::FindSwitchCaseSensitive(const UnicodeString Switch)
{
  UnicodeString Value;
  int32_t ParamsStart;
  int32_t ParamsCount;
  bool ValueSet;
  return FindSwitch(Switch, Value, ParamsStart, ParamsCount, true, ValueSet);
}

bool TOptions::FindSwitch(const UnicodeString Switch,
  TStrings *Params, int32_t ParamsMax)
{
  return DoFindSwitch(Switch, Params, ParamsMax, false);
}

bool TOptions::FindSwitchCaseSensitive(const UnicodeString Switch,
  TStrings *Params, int32_t ParamsMax)
{
  return DoFindSwitch(Switch, Params, ParamsMax, true);
}

bool TOptions::DoFindSwitch(const UnicodeString Switch,
  TStrings *Params, int32_t ParamsMax, bool CaseSensitive)
{
  UnicodeString Value;
  int32_t ParamsStart;
  int32_t ParamsCount;
  bool ValueSet;
  bool Result = FindSwitch(Switch, Value, ParamsStart, ParamsCount, CaseSensitive, ValueSet);
  if (Result)
  {
    int32_t AParamsCount;
    if (TryStrToInt(Value, AParamsCount) && (AParamsCount < ParamsCount))
    {
      ParamsCount = AParamsCount;
    }
    if ((ParamsMax >= 0) && (ParamsCount > ParamsMax))
    {
      ParamsCount = ParamsMax;
    }

    int32_t Index = 0;
    while (Index < ParamsCount)
    {
      Params->Add(GetParam(ParamsStart + Index));
      ++Index;
    }
    ParamsProcessed(ParamsStart, ParamsCount);
  }
  return Result;
}

UnicodeString TOptions::SwitchValue(const UnicodeString Switch,
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

bool TOptions::SwitchValue(const UnicodeString Switch, bool Default, bool DefaultOnNonExistence)
{
  bool Result;
  int64_t IntValue = 0;
  UnicodeString Value;
  if (!FindSwitch(Switch, Value))
  {
    Result = DefaultOnNonExistence;
  }
  else if (Value.IsEmpty())
  {
    Result = Default;
  }
  else if (::SameText(Value, L"on"))
  {
    Result = true;
  }
  else if (::SameText(Value, L"off"))
  {
    Result = false;
  }
  else if (::TryStrToInt64(Value, IntValue))
  {
    Result = (IntValue != 0);
  }
  else
  {
    throw Exception(FMTLOAD(URL_OPTION_BOOL_VALUE_ERROR, Value));
  }
  return Result;
}

bool TOptions::SwitchValue(const UnicodeString Switch, bool Default)
{
  return SwitchValue(Switch, Default, Default);
}

bool TOptions::UnusedSwitch(UnicodeString &Switch) const
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

bool TOptions::WasSwitchAdded(UnicodeString & Switch, UnicodeString & Value, wchar_t & SwitchMark) const
{
  bool Result =
    DebugAlwaysTrue(FOptions.size() > 0) &&
    (FOptions.back().Type == otSwitch);
  if (Result)
  {
    const TOption &Option = FOptions.back();
    Switch = Option.Name;
    Value = Option.Value;
    SwitchMark = Option.SwitchMark;
  }
  return Result;
}

void TOptions::ParamsProcessed(int32_t ParamsStart, int32_t ParamsCount)
{
  if (ParamsCount > 0)
  {
    DebugAssert((ParamsStart >= 0) && ((ParamsStart - ParamsCount + 1) <= FParamCount));

    size_t Index = 0;
    while ((Index < FOptions.size()) && (ParamsStart > 0))
    {
      if (FOptions[Index].Type == otParam)
      {
        --ParamsStart;

        if (ParamsStart == 0)
        {
          while (ParamsCount > 0)
          {
            DebugAssert(Index < FOptions.size());
            DebugAssert(FOptions[Index].Type == otParam);
            FOptions.erase(FOptions.begin() + Index);
            --FParamCount;
            --ParamsCount;
          }
        }
      }
      ++Index;
    }
  }
}

void TOptions::LogOptions(TLogOptionEvent OnLogOption)
{
  for (size_t Index = 0; Index < FOriginalOptions.size(); ++Index)
  {
    const TOption &Option = FOriginalOptions[Index];
    UnicodeString LogStr;
    switch (Option.Type)
    {
    case otParam:
      LogStr = FORMAT("Parameter: %s", Option.Value);
      DebugAssert(Option.Name.IsEmpty());
      break;

    case otSwitch:
      LogStr =
        FORMAT("Switch:    %s%s%s%s",
          FSwitchMarks[1], Option.Name, (Option.Value.IsEmpty() ? UnicodeString() : FSwitchValueDelimiters.SubString(1, 1)), Option.Value);
      break;

    default:
      DebugFail();
      break;
    }
    OnLogOption(LogStr);
  }
}
