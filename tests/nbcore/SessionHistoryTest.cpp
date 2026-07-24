#define NOMINMAX
#include <iostream>
#include <cassert>
#include <SessionHistory.h>
#include "FakeSessionContext.h"

using nb::EncodeSessionParam;
using nb::DecodeSessionParam;
using nb::SetSessionLogger;
using nb::LogSessionEvent;

int main()
{
  FakeSessionContext sessionContext;
  SetSessionLogger(&sessionContext);

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

  // --- Logging via injected context ---
  {
    LogSessionEvent(L"Session opened");
    LogSessionEvent(L"Transfer started");

    const auto & Entries = sessionContext.GetLogEntries();
    assert(Entries.size() == 2);
    assert(Entries[0].Text == L"Session opened");
    assert(Entries[1].Text == L"Transfer started");
  }

  // --- Compatibility overload still logs through explicit context ---
  {
    FakeSessionContext localContext;
    nb::LogSessionEvent(&localContext, L"Explicit context event");
    const auto & Entries = localContext.GetLogEntries();
    assert(Entries.size() == 1);
    assert(Entries[0].Text == L"Explicit context event");
  }

  // --- Null context should be ignored by compatibility overload ---
  {
    nb::LogSessionEvent(nullptr, L"Ignored");
    assert(sessionContext.GetLogEntries().size() == 2);
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
    auto decoded = DecodeSessionParam(L"");
    assert(!decoded.Valid);
    assert(decoded.SessionName.IsEmpty());
  }

  {
    auto decoded = DecodeSessionParam(L"random string");
    assert(!decoded.Valid);
  }

  {
    auto decoded = DecodeSessionParam(L"netbox://");
    assert(!decoded.Valid);
  }

  {
    auto decoded = DecodeSessionParam(L"netbox:LegacySession");
    assert(decoded.Valid);
    assert(decoded.SessionName == L"LegacySession");
    assert(decoded.RemoteDirectory.IsEmpty());
  }

  {
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
