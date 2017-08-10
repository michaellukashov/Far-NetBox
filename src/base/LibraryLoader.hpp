#pragma once

#include <string>
#include <Classes.hpp>
#include <Global.h>



class NB_CORE_EXPORT TLibraryLoader : public TObject
{
NB_DISABLE_COPY(TLibraryLoader)
public:
  explicit TLibraryLoader(UnicodeString libraryName, bool AllowFailure = false);
  explicit TLibraryLoader();
  virtual ~TLibraryLoader();

  void Load(UnicodeString LibraryName, bool AllowFailure = false);
  void Unload();
  FARPROC GetProcAddress(AnsiString ProcedureName) const;
  FARPROC GetProcAddress(intptr_t ProcedureOrdinal) const;
  bool Loaded() const { return FHModule != nullptr; }

private:
  HMODULE FHModule;
};


