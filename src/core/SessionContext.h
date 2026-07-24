#pragma once

#include "nbcore.h"

class TConfiguration;
class TSessionData;
class UnicodeString;
class TQueryParams;
class TStrings;

class ISessionContext
{
public:
  virtual ~ISessionContext() = default;

  // Configuration and session data access
  virtual TConfiguration * GetConfiguration() const = 0;
  virtual TSessionData * GetSessionData() const = 0;

  // Logging
  virtual void LogEvent(const UnicodeString & Str) = 0;
  virtual void LogEvent(int32_t Level, const UnicodeString & Str) = 0;

  // Error handling
  virtual void TerminalError(const UnicodeString & Msg) = 0;

  // Transaction management
  virtual void BeginTransaction() = 0;
  virtual void EndTransaction() = 0;
  virtual bool InTransaction() const = 0;

  // GUI processing
  virtual void ProcessGUI() = 0;

  // Escape key checking
  virtual bool CheckForEsc() = 0;

  // UI callbacks (delegate to TTerminal's existing events)
  virtual uint32_t QueryUser(const UnicodeString & Query,
                             TStrings * MoreMessages,
                             uint32_t Answers,
                             const TQueryParams * Params,
                             int32_t QueryType = 0) = 0;

  virtual bool PromptUser(const UnicodeString & Name,
                          const UnicodeString & Instructions,
                          TStrings * Prompts,
                          TStrings * Results) = 0;

  virtual void DisplayBanner(const UnicodeString & Banner) = 0;
};