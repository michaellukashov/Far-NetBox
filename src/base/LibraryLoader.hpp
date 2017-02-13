#pragma once

#include <string>
#include <Classes.hpp>
#include <Global.h>



class TLibraryLoader : public TObject
{
NB_DISABLE_COPY(TLibraryLoader)
public:
  explicit TLibraryLoader(const UnicodeString & libraryName, bool AllowFailure = false);
  explicit TLibraryLoader();
  virtual ~TLibraryLoader();

  void Load(const UnicodeString & LibraryName, bool AllowFailure = false);
  void Unload();
  FARPROC GetProcAddress(const AnsiString & ProcedureName);
  FARPROC GetProcAddress(intptr_t ProcedureOrdinal);
  bool Loaded() const { return FHModule != nullptr; }

private:
  HMODULE FHModule;
};


