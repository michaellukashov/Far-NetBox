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

bool TWinConfiguration::ValidateMasterPassword(UnicodeString value) {
  DebugAssert(GetUseMasterPassword());
  DebugAssert(!FMasterPasswordVerifier.IsEmpty());
  bool Result = AES256Verify(value, HexToBytes(FMasterPasswordVerifier));
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
