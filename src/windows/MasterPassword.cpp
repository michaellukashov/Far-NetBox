#pragma hdrstop

#include "WinConfiguration.h"
#include "Cryptography.h"
#include <Sysutils.hpp>

// Minimal inline RecryptPasswords that only calls the base class.
// This avoids the VCL-dependent TTerminalManager inclusion from TerminalManager.h.
namespace {
void MasterPasswordRecryptPasswords(TWinConfiguration * Config, TStrings * RecryptPasswordErrors) {
  Config->RecryptPasswords(RecryptPasswordErrors);
}
}

bool TWinConfiguration::GetUseMasterPassword() {
  return FUseMasterPassword;
}

void TWinConfiguration::ChangeMasterPassword(
    UnicodeString value, TStrings * RecryptPasswordErrors) {
  RawByteString Verifier;
  AES256CreateVerifier(value, Verifier);
  FMasterPasswordVerifier = BytesToHex(Verifier);
  FPlainMasterPasswordEncrypt.SetValue(value);
  FUseMasterPassword = true;
  try__finally
  {
    MasterPasswordRecryptPasswords(this, RecryptPasswordErrors);
  }
  __finally
  {
    FPlainMasterPasswordDecrypt.SetValue(value);
  } end_try__finally
}

bool TWinConfiguration::ValidateMasterPassword(UnicodeString value, bool CountAttempt) {
  DebugAssert(GetUseMasterPassword());
  DebugAssert(!FMasterPasswordVerifier.IsEmpty());

  if (CountAttempt)
  {
    const uint32_t Failures = FValidationTracker.ConsecutiveFailures.load();
    if (Failures >= TValidationAttemptTracker::MaxAttempts)
    {
      const uint32_t Now = GetTickCount();
      const uint32_t Last = FValidationTracker.LastAttemptTime.load();
      const uint32_t Elapsed = (Now - Last) / 1000;
      if (Elapsed < TValidationAttemptTracker::LockoutSeconds)
      {
        throw Exception(FORMAT(
          L"Too many failed attempts. Please wait %d seconds.",
          TValidationAttemptTracker::LockoutSeconds - Elapsed));
      }
      FValidationTracker.ConsecutiveFailures.store(0);
    }
  }

  bool Result = AES256Verify(value, HexToBytes(FMasterPasswordVerifier));

  if (CountAttempt)
  {
    if (Result)
      FValidationTracker.ConsecutiveFailures.store(0);
    else
    {
      FValidationTracker.ConsecutiveFailures.fetch_add(1);
      FValidationTracker.LastAttemptTime.store(GetTickCount());
    }
  }

  return Result;
}

void TWinConfiguration::ClearMasterPassword(TStrings * RecryptPasswordErrors) {
  FMasterPasswordVerifier = L"";
  FUseMasterPassword = false;
  FPlainMasterPasswordEncrypt.Clear();
  try__finally
  {
    MasterPasswordRecryptPasswords(this, RecryptPasswordErrors);
  }
  __finally
  {
    FPlainMasterPasswordDecrypt.Clear();
  } end_try__finally
}
