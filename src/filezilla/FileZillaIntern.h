
#pragma once

#include <nbsystem.h>

#undef PostMessage

class TFileZillaIntf;

class TFileZillaIntern // : public TObject
{
CUSTOM_MEM_ALLOCATION_IMPL
NB_DISABLE_COPY(TFileZillaIntern)
public:
  explicit TFileZillaIntern(TFileZillaIntf * AOwner) noexcept;

  bool PostMessage(WPARAM wParam, LPARAM lParam) const;
  CString GetOption(int32_t OptionID) const;
  int32_t GetOptionVal(int32_t OptionID) const;

  inline const TFileZillaIntf * GetOwner() const { return FOwner; }

  int32_t GetDebugLevel() const;
  void SetDebugLevel(int32_t DebugLevel);

protected:
  TFileZillaIntf * FOwner{nullptr};
  int32_t FDebugLevel{0};
};

