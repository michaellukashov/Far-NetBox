#include "LibraryLoader.hpp"


TLibraryLoader::TLibraryLoader(UnicodeString LibraryName, bool AllowFailure) :
  FHModule(nullptr)
{
  Load(LibraryName, AllowFailure);
}

TLibraryLoader::TLibraryLoader() :
  FHModule(nullptr)
{
}

TLibraryLoader::~TLibraryLoader()
{
  Unload();
}

void TLibraryLoader::Load(UnicodeString LibraryName, bool AllowFailure)
{
  DebugAssert(FHModule == nullptr);

  // Loading library
  FHModule = ::LoadLibrary(LibraryName.c_str());

  if (!AllowFailure)
  {
    DebugAssert(FHModule != nullptr);
  }
}

void TLibraryLoader::Unload()
{
  if (FHModule != nullptr)
  {
    ::FreeLibrary(FHModule);
    FHModule = nullptr;
  }
}

// Get procedure address from loaded library by name
FARPROC TLibraryLoader::GetProcAddress(const AnsiString & ProcedureName) const
{
  return ::GetProcAddress(FHModule, ProcedureName.c_str());
}

// Get procedure address from loaded library by ordinal value
FARPROC TLibraryLoader::GetProcAddress(intptr_t ProcedureOrdinal) const
{
  return ::GetProcAddress(FHModule, (LPCSTR)nullptr + ProcedureOrdinal);
}


