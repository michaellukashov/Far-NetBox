
#ifndef FileZillaInternH
#define FileZillaInternH

class TFileZillaIntf;

class TFileZillaIntern : public TObject
{
NB_DISABLE_COPY(TFileZillaIntern)
public:
  virtual TObjectClassId GetKind() const { return OBJECT_CLASS_TFileZillaIntern; }
  static bool classof(const TObject * Obj)
  {
    TObjectClassId Kind = Obj->GetKind();
    return
      Kind == OBJECT_CLASS_TFileZillaIntern;
  }
public:
  explicit TFileZillaIntern(TFileZillaIntf * AOwner);

  bool PostMessage(WPARAM wParam, LPARAM lParam) const;
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
