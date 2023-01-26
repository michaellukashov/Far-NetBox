
#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include "ProgParams.h"

// #pragma package(smart_init)

std::unique_ptr<TProgramParams> ProgramParamsOwner;

TProgramParams * TProgramParams::Instance()
{
  if (ProgramParamsOwner.get() == nullptr)
  {
    ProgramParamsOwner = std::make_unique<TProgramParams>();
  }
  return ProgramParamsOwner.get();
}

TProgramParams::TProgramParams() noexcept
{
  Init(""); // (CmdLine);
}

TProgramParams::TProgramParams(const UnicodeString & CmdLine) noexcept
{
  Init(CmdLine);
}

void TProgramParams::Init(const UnicodeString & CmdLine)
{
  UnicodeString CommandLine = CmdLine;

  UnicodeString Param;
  CutToken(CommandLine, Param);
  Parse(CommandLine);
}

UnicodeString TProgramParams::FormatSwitch(const UnicodeString & Switch)
{
  return FORMAT("/%s", Switch);
}
