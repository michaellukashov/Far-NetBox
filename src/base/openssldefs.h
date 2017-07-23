#pragma once

// Resolve ambiguity with OpenSSL and putty
#undef SHA_Init
#undef SHA_Final
#undef SHA256_Init
#undef SHA256_Final
#undef SHA512_Init
#undef SHA512_Final

#undef sk_new
