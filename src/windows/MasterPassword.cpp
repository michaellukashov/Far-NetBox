#pragma hdrstop

#include "WinConfiguration.h"
#include "Cryptography.h"

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
  FPlainMasterPasswordEncrypt = value;
  FUseMasterPassword = true;
  MasterPasswordRecryptPasswords(this, RecryptPasswordErrors);
  FPlainMasterPasswordDecrypt = value;
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
  Shred(FPlainMasterPasswordEncrypt);
  MasterPasswordRecryptPasswords(this, RecryptPasswordErrors);
  Shred(FPlainMasterPasswordDecrypt);
}
