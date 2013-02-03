//------------------------------------------------------------------------------
#pragma once
#define ProgParamsH

#include <Option.h>
//------------------------------------------------------------------------------
class TProgramParams : public TOptions
{
public:
  static TProgramParams * Instance();

  TProgramParams();
};
//------------------------------------------------------------------------------
