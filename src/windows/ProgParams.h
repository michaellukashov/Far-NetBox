#pragma once
#define ProgParamsH

#include <Option.h>

class NB_CORE_EXPORT TProgramParams : public TOptions
{
public:
  // static TProgramParams * Instance();

  explicit TProgramParams();
  explicit TProgramParams(const UnicodeString & CmdLine);

private:
  void Init(const UnicodeString & CmdLine);
};

