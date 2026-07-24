#define NOMINMAX
#include <iostream>
#include <cassert>
#include <Configuration.h>

class TMockConfiguration final : public TConfiguration
{
public:
  explicit TMockConfiguration() noexcept :
    TConfiguration(OBJECT_CLASS_TConfiguration)
  {
  }

  virtual UnicodeString TemporaryDir(bool /*Mask*/ = false) const override
  {
    return L"";
  }
};

int main()
{
  // --- SilentMode default value ---
  {
    TMockConfiguration Config;
    assert(Config.GetSilentMode() == false);
    std::wcout << L"Test: SilentMode default value: " << (Config.GetSilentMode() ? L"true" : L"false") << std::endl;
  }

  // --- SilentMode set/get round-trip ---
  {
    TMockConfiguration Config;
    Config.SetSilentMode(true);
    assert(Config.GetSilentMode() == true);
    std::wcout << L"Test: SilentMode after set: " << (Config.GetSilentMode() ? L"true" : L"false") << std::endl;

    Config.SetSilentMode(false);
    assert(Config.GetSilentMode() == false);
    std::wcout << L"Test: SilentMode after reset: " << (Config.GetSilentMode() ? L"true" : L"false") << std::endl;
  }

  std::wcout << L"All configuration tests passed." << std::endl;
  return 0;
}
