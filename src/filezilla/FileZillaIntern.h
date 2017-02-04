#ifndef FileZillaInternH
#define FileZillaInternH

#include <headers.hpp>

class TFileZillaIntf;

class TFileZillaIntern // : public TObject
{
CUSTOM_MEM_ALLOCATION_IMPL
NB_DISABLE_COPY(TFileZillaIntern)
public:
  static bool classof(const TFileZillaIntern * Obj)
  {
    return
      Obj->GetKind() == OBJECT_CLASS_TFileZillaIntern;
  }
public:
  explicit TFileZillaIntern(TFileZillaIntf * AOwner);

  bool FZPostMessage(WPARAM wParam, LPARAM lParam) const;
  CString GetOption(int OptionID) const;
  int GetOptionVal(int OptionID) const;

  inline const TFileZillaIntf * GetOwner() const { return FOwner; }

  int GetDebugLevel() const;
  void SetDebugLevel(int DebugLevel);

protected:
  TFileZillaIntf * FOwner;
  int FDebugLevel;
};

#endif // FileZillaInternH
