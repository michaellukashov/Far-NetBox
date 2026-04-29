#define NOMINMAX
#include <iostream>
#include <cassert>
#include <SessionHistory.h>

using nb::EncodeSessionParam;
using nb::DecodeSessionParam;

int main()
{
  // --- Basic encode/decode round-trip ---
  {
    UnicodeString sessionName = L"MySession";
    UnicodeString remoteDir = L"/home/user/docs";
    UnicodeString encoded = EncodeSessionParam(sessionName, remoteDir);
    assert(encoded == L"netbox://MySession/home/user/docs");

    auto decoded = DecodeSessionParam(encoded);
    assert(decoded.Valid);
    assert(decoded.SessionName == sessionName);
    assert(decoded.RemoteDirectory == remoteDir);
  }

  // --- Empty remote directory ---
  {
    UnicodeString sessionName = L"RootSession";
    UnicodeString remoteDir = L"";
    UnicodeString encoded = EncodeSessionParam(sessionName, remoteDir);
    assert(encoded == L"netbox://RootSession");

    auto decoded = DecodeSessionParam(encoded);
    assert(decoded.Valid);
    assert(decoded.SessionName == sessionName);
    assert(decoded.RemoteDirectory.IsEmpty());
  }

  // --- Unicode session names (Russian) ---
  {
    UnicodeString sessionName = L"\u0422\u0435\u0441\u0442"; // "Test" in Russian
    UnicodeString remoteDir = L"/data";
    UnicodeString encoded = EncodeSessionParam(sessionName, remoteDir);

    auto decoded = DecodeSessionParam(encoded);
    assert(decoded.Valid);
    assert(decoded.SessionName == sessionName);
    assert(decoded.RemoteDirectory == remoteDir);
  }

  // --- Unicode session names (Polish) ---
  {
    UnicodeString sessionName = L"\u0141\u00f3\u0064\u017a"; // "Lodz" with Polish chars
    UnicodeString remoteDir = L"/pliki";
    UnicodeString encoded = EncodeSessionParam(sessionName, remoteDir);

    auto decoded = DecodeSessionParam(encoded);
    assert(decoded.Valid);
    assert(decoded.SessionName == sessionName);
    assert(decoded.RemoteDirectory == remoteDir);
  }

  // --- Special characters in paths (|, &, spaces) ---
  {
    UnicodeString sessionName = L"SpecialSession";
    UnicodeString remoteDir = L"/path with spaces/dir|name&test";
    UnicodeString encoded = EncodeSessionParam(sessionName, remoteDir);

    auto decoded = DecodeSessionParam(encoded);
    assert(decoded.Valid);
    assert(decoded.SessionName == sessionName);
    assert(decoded.RemoteDirectory == remoteDir);
  }

  // --- Special characters in session name ---
  {
    UnicodeString sessionName = L"Session Name+Test";
    UnicodeString remoteDir = L"/root";
    UnicodeString encoded = EncodeSessionParam(sessionName, remoteDir);

    auto decoded = DecodeSessionParam(encoded);
    assert(decoded.Valid);
    assert(decoded.SessionName == sessionName);
    assert(decoded.RemoteDirectory == remoteDir);
  }

  // --- Very long session name and path ---
  {
    UnicodeString sessionName;
    for (int i = 0; i < 200; ++i)
    {
      sessionName += L"a";
    }
    UnicodeString remoteDir;
    for (int i = 0; i < 10; ++i)
    {
      remoteDir += L"/very/long/path/segment";
    }
    UnicodeString encoded = EncodeSessionParam(sessionName, remoteDir);

    auto decoded = DecodeSessionParam(encoded);
    assert(decoded.Valid);
    assert(decoded.SessionName == sessionName);
    assert(decoded.RemoteDirectory == remoteDir);
  }

  // --- Malformed param strings (graceful degradation) ---
  {
    // Empty string
    auto decoded = DecodeSessionParam(L"");
    assert(!decoded.Valid);
    assert(decoded.SessionName.IsEmpty());
  }

  {
    // No prefix at all
    auto decoded = DecodeSessionParam(L"random string");
    assert(!decoded.Valid);
  }

  {
    // Only scheme, no session name
    auto decoded = DecodeSessionParam(L"netbox://");
    assert(!decoded.Valid);
  }

  {
    // Old legacy format without remote directory
    auto decoded = DecodeSessionParam(L"netbox:LegacySession");
    assert(decoded.Valid);
    assert(decoded.SessionName == L"LegacySession");
    assert(decoded.RemoteDirectory.IsEmpty());
  }

  {
    // Old legacy format with remote directory
    auto decoded = DecodeSessionParam(L"netbox:LegacySession\1/legacy/dir");
    assert(decoded.Valid);
    assert(decoded.SessionName == L"LegacySession");
    assert(decoded.RemoteDirectory == L"/legacy/dir");
  }

  // --- Path with slashes but no remote dir (session name only) ---
  {
    UnicodeString sessionName = L"NoDirSession";
    UnicodeString remoteDir = L"";
    UnicodeString encoded = EncodeSessionParam(sessionName, remoteDir);
    assert(encoded == L"netbox://NoDirSession");

    auto decoded = DecodeSessionParam(encoded);
    assert(decoded.Valid);
    assert(decoded.SessionName == sessionName);
    assert(decoded.RemoteDirectory.IsEmpty());
  }

  std::wcout << L"All SessionHistory tests passed." << std::endl;
  return 0;
}
