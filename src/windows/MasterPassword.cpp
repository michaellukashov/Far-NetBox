#pragma hdrstop

#include "WinConfiguration.h"
#include "Cryptography.h"
#include <Sysutils.hpp>
#include "Common.h"
#include <TextsWin.h>

// Minimal inline RecryptPasswords that only calls the base class.
// This avoids the VCL-dependent TTerminalManager inclusion from TerminalManager.h.
namespace {

void MasterPasswordRecryptPasswords(TWinConfiguration * Config, TStrings * RecryptPasswordErrors) {
  Config->RecryptPasswords(RecryptPasswordErrors);
}

}
