
#ifndef FileZillaInternH
#define FileZillaInternH

#include "ApiLog.h"

class TFileZillaIntf;

class TFileZillaIntern : public CApiLog
{
NB_DECLARE_CLASS(TFileZillaIntern)
NB_DISABLE_COPY(TFileZillaIntern)
public:
  explicit TFileZillaIntern(TFileZillaIntf * AOwner);

  virtual BOOL PostMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam) const;

  inline const TFileZillaIntf * GetOwner() const { return FOwner; }

protected:
  TFileZillaIntf * FOwner;
};

#endif // FileZillaInternH
