#include <vcl.h>
#pragma hdrstop

#include <Common.h>
#include "ProgParams.h"
//------------------------------------------------------------------------------
#pragma package(smart_init)
//------------------------------------------------------------------------------
// unique_ptr-like class
class TProgramParamsOwner : public TObject
{
public:
  TProgramParamsOwner() :
    FProgramParams(nullptr)
  {
  }

  ~TProgramParamsOwner()
  {
    delete FProgramParams;
  }

  TProgramParams * Get()
  {
    if (FProgramParams == nullptr)
    {
      FProgramParams = new TProgramParams();
    }
    return FProgramParams;
  }

private:
  TProgramParams * FProgramParams;
};
//------------------------------------------------------------------------------
// TProgramParamsOwner ProgramParamsOwner;
//------------------------------------------------------------------------------
// TProgramParams * TProgramParams::Instance()
// {
  // return ProgramParamsOwner.Get();
// }
//---------------------------------------------------------------------------
TProgramParams::TProgramParams()
{
  Init(L"");
}
//---------------------------------------------------------------------------
TProgramParams::TProgramParams(const UnicodeString & CmdLine)
{
  Init(CmdLine);
}
//---------------------------------------------------------------------------
void TProgramParams::Init(const UnicodeString & CmdLine)
{
  UnicodeString CommandLine = CmdLine;

  UnicodeString Param;
  CutToken(CommandLine, Param);
  while (CutToken(CommandLine, Param))
  {
    Add(Param);
  }
}
