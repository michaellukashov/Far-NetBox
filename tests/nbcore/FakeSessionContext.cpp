#include "FakeSessionContext.h"

TConfiguration * FakeSessionContext::GetConfiguration() const
{
  return FConfiguration;
}

TSessionData * FakeSessionContext::GetSessionData() const
{
  return FSessionData;
}

void FakeSessionContext::LogEvent(const UnicodeString & Str)
{
  FLogEntries.push_back({0, Str});
}

void FakeSessionContext::LogEvent(int32_t Level, const UnicodeString & Str)
{
  FLogEntries.push_back({Level, Str});
}

void FakeSessionContext::TerminalError(const UnicodeString & Msg)
{
  FErrors.push_back(Msg);
}

void FakeSessionContext::BeginTransaction()
{
  FInTransaction = true;
}

void FakeSessionContext::EndTransaction()
{
  FInTransaction = false;
}

bool FakeSessionContext::InTransaction() const
{
  return FInTransaction;
}

void FakeSessionContext::ProcessGUI()
{
}

bool FakeSessionContext::CheckForEsc() const
{
  return FCheckForEsc;
}

uint32_t FakeSessionContext::QueryUser(const UnicodeString &,
                                       TStrings *,
                                       uint32_t,
                                       const TQueryParams *,
                                       int32_t)
{
  return FQueryUserResult;
}

bool FakeSessionContext::PromptUser(const UnicodeString &,
                                    const UnicodeString &,
                                    TStrings *,
                                    TStrings *)
{
  return FPromptUserResult;
}

void FakeSessionContext::DisplayBanner(const UnicodeString & Banner)
{
  FBanners.push_back(Banner);
}

void FakeSessionContext::SetConfiguration(TConfiguration * Configuration)
{
  FConfiguration = Configuration;
}

void FakeSessionContext::SetSessionData(TSessionData * SessionData)
{
  FSessionData = SessionData;
}

void FakeSessionContext::SetCheckForEsc(bool Value)
{
  FCheckForEsc = Value;
}

void FakeSessionContext::SetQueryUserResult(uint32_t Value)
{
  FQueryUserResult = Value;
}

void FakeSessionContext::SetPromptUserResult(bool Value)
{
  FPromptUserResult = Value;
}

const std::vector<FakeSessionContext::TLogEntry> & FakeSessionContext::GetLogEntries() const
{
  return FLogEntries;
}

const std::vector<UnicodeString> & FakeSessionContext::GetErrors() const
{
  return FErrors;
}

const std::vector<UnicodeString> & FakeSessionContext::GetBanners() const
{
  return FBanners;
}
