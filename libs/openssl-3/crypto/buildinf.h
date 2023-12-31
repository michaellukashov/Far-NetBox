/*
 * WARNING: do not edit!
 * Generated by util/mkbuildinf.pl
 *
 * Copyright 2014-2017 The OpenSSL Project Authors. All Rights Reserved.
 *
 * Licensed under the Apache License 2.0 (the "License").  You may not use
 * this file except in compliance with the License.  You can obtain a copy
 * in the file LICENSE in the source distribution or at
 * https://www.openssl.org/source/license.html
 */

#if defined(_WIN64) || defined(OPENSSL_SYS_WIN64)
#define PLATFORM "platform: VC-WIN64A"
#else
#define PLATFORM "platform: VC-WIN32"
#endif
#define DATE "built on: Sun Dec 17 09:10:48 2023 UTC"

/*
 * Generate compiler_flags as an array of individual characters. This is a
 * workaround for the situation where CFLAGS gets too long for a C90 string
 * literal
 */
#if defined(_WIN64) || defined(OPENSSL_SYS_WIN64)
static const char compiler_flags[] = {
    'c','o','m','p','i','l','e','r',':',' ','c','l',' ',' ','/','Z',
    'i',' ','/','F','d','o','s','s','l','_','s','t','a','t','i','c',
    '.','p','d','b',' ','/','G','s','0',' ','/','G','F',' ','/','G',
    'y',' ','/','M','D',' ','/','W','3',' ','/','w','d','4','0','9',
    '0',' ','/','n','o','l','o','g','o',' ','/','O','2',' ','-','D',
    'L','_','E','N','D','I','A','N',' ','-','D','O','P','E','N','S',
    'S','L','_','P','I','C','\0'
};
#else
static const char compiler_flags[] = {
    'c','o','m','p','i','l','e','r',':',' ','c','l',' ',' ','/','Z',
    'i',' ','/','F','d','o','s','s','l','_','s','t','a','t','i','c',
    '.','p','d','b',' ','/','G','s','0',' ','/','G','F',' ','/','G',
    'y',' ','/','M','D',' ','/','W','3',' ','/','w','d','4','0','9',
    '0',' ','/','n','o','l','o','g','o',' ','/','O','2',' ','-','D',
    'L','_','E','N','D','I','A','N',' ','-','D','"','O','P','E','N',
    'S','S','L','_','B','U','I','L','D','I','N','G','_','O','P','E',
    'N','S','S','L','"',' ','-','D','"','O','P','E','N','S','S','L',
    '_','S','Y','S','_','W','I','N','3','2','"',' ','-','D','"','W',
    'I','N','3','2','_','L','E','A','N','_','A','N','D','_','M','E',
    'A','N','"',' ','-','D','"','U','N','I','C','O','D','E','"',' ',
    '-','D','"','_','U','N','I','C','O','D','E','"',' ','-','D','"',
    '_','C','R','T','_','S','E','C','U','R','E','_','N','O','_','D',
    'E','P','R','E','C','A','T','E','"',' ','-','D','"','_','W','I',
    'N','S','O','C','K','_','D','E','P','R','E','C','A','T','E','D',
    '_','N','O','_','W','A','R','N','I','N','G','S','"',' ','-','D',
    '"','N','D','E','B','U','G','"','\0'
};
#endif
