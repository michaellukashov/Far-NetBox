#pragma once
#define ProgParamsH

#include <Option.h>

class NB_CORE_EXPORT TProgramParams : public TOptions
{
public:
  // static TProgramParams * Instance();

  explicit TProgramParams();
  explicit TProgramParams(UnicodeString CmdLine);

private:
  void Init(UnicodeString CmdLine);
};

