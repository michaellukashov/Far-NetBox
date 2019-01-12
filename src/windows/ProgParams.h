//---------------------------------------------------------------------------
#pragma once
#define ProgParamsH

#include <Option.h>
//---------------------------------------------------------------------------
class NB_CORE_EXPORT TProgramParams : public TOptions
{
public:
  static TProgramParams * Instance();

  explicit TProgramParams() noexcept;
  explicit TProgramParams(const UnicodeString CmdLine) noexcept;

  static UnicodeString FormatSwitch(UnicodeString Switch);

private:
  void Init(const UnicodeString CmdLine);
};
//---------------------------------------------------------------------------
