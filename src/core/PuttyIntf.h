#pragma once

#include <nbcore.h>

NB_CORE_EXPORT void PuttyInitialize();
NB_CORE_EXPORT void PuttyFinalize();

NB_CORE_EXPORT void DontSaveRandomSeed();

#include "PuttyTools.h"

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
// Defined in marshal.h - Conflicts with xml.xmldom.hpp
#undef get_data

  extern CRITICAL_SECTION noise_section;
}

UnicodeString GetCipherName(const ssh_cipher * Cipher);
UnicodeString GetCompressorName(const ssh_compressor * Compressor);
UnicodeString GetDecompressorName(const ssh_decompressor * Decompressor);
void PuttyDefaults(Conf * conf);
int32_t GetCipherGroup(const ssh_cipher * TheCipher);

class TSecureShell;
struct ScpSeat : public Seat
{
  CUSTOM_MEM_ALLOCATION_IMPL
  TSecureShell * SecureShell{nullptr};

  explicit ScpSeat(TSecureShell * SecureShell);
};

extern THierarchicalStorage * PuttyStorage;


