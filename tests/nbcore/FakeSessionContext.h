#pragma once

#include <SessionContext.h>

#include <vector>

class FakeSessionContext final : public ISessionContext
{
public:
  TConfiguration * GetConfiguration() const override;
  TSessionData * GetSessionData() const override;

  void LogEvent(const UnicodeString & Str) override;
  void LogEvent(int32_t Level, const UnicodeString & Str) override;

  void TerminalError(const UnicodeString & Msg) override;

  void BeginTransaction() override;
  void EndTransaction() override;
  bool InTransaction() const override;

  void ProcessGUI() override;
  bool CheckForEsc() const override;

  uint32_t QueryUser(const UnicodeString & Query,
                     TStrings * MoreMessages,
                     uint32_t Answers,
                     const TQueryParams * Params,
                     int32_t QueryType = 0) override;

  bool PromptUser(const UnicodeString & Name,
                  const UnicodeString & Instructions,
                  TStrings * Prompts,
                  TStrings * Results) override;

  void DisplayBanner(const UnicodeString & Banner) override;

  void SetConfiguration(TConfiguration * Configuration);
  void SetSessionData(TSessionData * SessionData);
  void SetCheckForEsc(bool Value);
  void SetQueryUserResult(uint32_t Value);
  void SetPromptUserResult(bool Value);

  struct TLogEntry
  {
    int32_t Level{0};
    UnicodeString Text;
  };

  const std::vector<TLogEntry> & GetLogEntries() const;
  const std::vector<UnicodeString> & GetErrors() const;
  const std::vector<UnicodeString> & GetBanners() const;

private:
  TConfiguration * FConfiguration{nullptr};
  TSessionData * FSessionData{nullptr};
  bool FInTransaction{false};
  bool FCheckForEsc{false};
  uint32_t FQueryUserResult{0};
  bool FPromptUserResult{false};
  std::vector<TLogEntry> FLogEntries;
  std::vector<UnicodeString> FErrors;
  std::vector<UnicodeString> FBanners;
};
