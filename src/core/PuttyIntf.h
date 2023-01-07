#pragma once
#include "PuttyTools.h"

NB_CORE_EXPORT void PuttyInitialize();
NB_CORE_EXPORT void PuttyFinalize();

NB_CORE_EXPORT void DontSaveRandomSeed();

#ifndef MPEXT
#define MPEXT
#endif
extern "C"
{
#include <putty.h>
// To rename ssh1_cipheralg::new member, what is a keyword in C++
#define new _new_
#include <ssh.h>
#undef new
#include <puttyexp.h>
#include <proxy\proxy.h>
#include <storage.h>
// Defined in misc.h - Conflicts with std::min/max
#undef min
#undef max

  extern CRITICAL_SECTION noise_section;
}

UnicodeString GetCipherName(const ssh_cipher * Cipher);
UnicodeString GetCompressorName(const ssh_compressor * Compressor);
UnicodeString GetDecompressorName(const ssh_decompressor * Decompressor);
void PuttyDefaults(Conf * conf);

class TSecureShell;
struct ScpSeat : public Seat
{
  TSecureShell * SecureShell;

  ScpSeat(TSecureShell * SecureShell);
};

extern THierarchicalStorage * PuttyStorage;


