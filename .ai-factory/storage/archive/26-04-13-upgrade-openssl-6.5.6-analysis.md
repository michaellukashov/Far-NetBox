# OpenSSL 6.5.5 → 6.5.6 Analysis Results

**Date:** 2026-04-12
**WinSCP source:** D:\Projects\NetBox\winscp-master\libs\openssl\
**NetBox target:** D:\Projects\NetBox\NetBox-dev\libs\openssl-3\

## Summary

| Category | Count |
|----------|-------|
| SAME (identical) | 76 |
| DIFFER (need update) | 1,232 |
| WINSCP_ONLY (need add) | 0 |
| NETBOX_ONLY (potential remove) | 455 |

**Total WinSCP files: 1,308 | Total NetBox files: 1,763**

## Key Findings

1. **Zero files need to be ADDED** — NetBox's openssl-3 is a superset of WinSCP's openssl in terms of file presence
2. **1,232 of 1,308 WinSCP files (94%) differ** from NetBox counterparts — massive update scope
3. **455 files in NetBox have no WinSCP counterpart** — many are legacy OpenSSL files, QUIC support, FIPS provider code, or disabled algorithms
4. **The `providers/` directory** is the most critical area — both have it, most files differ, NetBox has additional FIPS/legacy provider files
5. **76 files are byte-identical** — mostly generated DER files, small utilities, specific headers

## NETBOX_ONLY Files (455 potential removal candidates)

### Legacy OpenSSL files removed in WinSCP 6.5.6:
- `crypto/async/arch/async_null.c`, `async_posix.c`
- `crypto/bf/bf_enc.c`, `bf_locl.h`, `blowfish.h`
- `crypto/bio/b_addr.c`, `b_dump.c`, `b_print.c`, `b_sock.c`, `b_sock2.c`, `bf_lbuf.c`, `bio.h`, `bio_lcl.h`
- `crypto/bn/asm/x86_64-gcc.c`, `bn_asm.c`, `bn_lcl.h`, `bn_ppc.c`, `bn_s390x.c`, `bn_sparc.c`, `bn_x931p.c`, `rsa_sup_mul.c`, `rsaz_exp.c`, `rsaz_exp_x2.c`
- `crypto/camellia/` — entire directory (8 files)
- `crypto/engine/` — entire directory (~30 files, legacy engine support)
- `crypto/kdf/` — hkdf.c, scrypt.c, tls1_prf.c (moved to providers in newer OpenSSL)
- `crypto/rand/drbg_*.c`, `md_rand.c`, `randfile.c` (moved to providers)
- `crypto/store/loader_file.c`, `store_strings.c`

### Crypto algorithms removed/disabled in WinSCP build:
- `crypto/blake2/` — 6 files
- `crypto/md2/` — 2 files
- `crypto/mdc2/` — 2 files
- `crypto/poly1305/` — extra implementation files
- `crypto/rc5/` — 6 files
- `crypto/seed/` — 6 files
- `crypto/siphash/` — extra files
- `crypto/whrlpool/` — 3 files

### QUIC support (not in WinSCP's build):
- `ssl/quic/` — ~40 files
- `ssl/event_queue.c`, `ssl/priority_queue.c`
- Various `include/internal/quic_*.h` files

### FIPS provider files:
- `providers/fips/` — 5 files
- `providers/common/securitycheck_fips.c`
- `providers/legacyprov.c`

### Legacy cipher providers:
- `providers/implementations/ciphers/cipher_blowfish*`, `cipher_camellia*`, `cipher_cast*`, `cipher_des*`, `cipher_idea*`, `cipher_rc2*`, `cipher_rc4*`, `cipher_rc5*`, `cipher_seed*`
- `providers/implementations/digests/md2_prov.c`, `md4_prov.c`, `mdc2_prov.c`, `wp_prov.c`
