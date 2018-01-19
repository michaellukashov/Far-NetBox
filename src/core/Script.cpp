
#include <vcl.h>
#pragma hdrstop

#include <Sysutils.hpp>

#include "Script.h"
  FCommands->Register(L"cp", SCRIPT_CP_DESC, SCRIPT_CP_HELP, &CpProc, 2, -1, false);
void __fastcall TScript::DoMvOrCp(TScriptProcParams * Parameters, TFSCapability Capability, bool Cp)
  if (!FTerminal->IsCapable[Capability])
  {
    NotSupported();
  }

  TStrings * FileList =
    CreateFileList(Parameters, 1, Parameters->ParamCount - 1, TFileListType(fltMask | fltQueryServer));
    if (Cp)
    {
      FTerminal->CopyFiles(FileList, TargetDirectory, FileMask);
    }
    else
    {
      FTerminal->MoveFiles(FileList, TargetDirectory, FileMask);
    }
  }
void __fastcall TScript::MvProc(TScriptProcParams * Parameters)
{
  DoMvOrCp(Parameters, fcRemoteMove, false);
}
//---------------------------------------------------------------------------
void __fastcall TScript::CpProc(TScriptProcParams * Parameters)
{
  DoMvOrCp(Parameters, fcRemoteCopy, true);
}
//---------------------------------------------------------------------------
  FCommands->Register(L"open", SCRIPT_OPEN_DESC, SCRIPT_OPEN_HELP10, &OpenProc, 0, -1, true);

const wchar_t * ToggleNames[] = { L"off", L"on" };
  UnicodeString OptionWithParameters;
        OptionWithParameters = L"";
          RawParam = FORMAT(L"%s%s=%s", (SwitchMark, Switch, PasswordMask));
        else if (TSessionData::IsOptionWithParameters(Switch))
        {
          OptionWithParameters = Switch;
        }
        if (!OptionWithParameters.IsEmpty())
        {
          if (TSessionData::MaskPasswordInOptionParameter(OptionWithParameters, Param))
          {
            RawParam = Param;
            AnyMaskedParam = true;
          }
        }

        DataWithFingerprint->MaskPasswords();
