#pragma once

#include <string>
#include <Classes.hpp>
#include <Global.h>



class NB_CORE_EXPORT TLibraryLoader : public TObject
{
NB_DISABLE_COPY(TLibraryLoader)
public:
  explicit TLibraryLoader(const UnicodeString & libraryName, bool AllowFailure = false) noexcept;
  explicit TLibraryLoader() noexcept;
  virtual ~TLibraryLoader() noexcept;

  void Load(const UnicodeString & LibraryName, bool AllowFailure = false);
  void Unload();
  FARPROC GetProcAddress(AnsiString ProcedureName) const;
  FARPROC GetProcAddress(int32_t ProcedureOrdinal) const;
  bool Loaded() const { return FHModule != nullptr; }

private:
  HMODULE FHModule;
};


