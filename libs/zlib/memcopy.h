/* memcopy.h -- inline functions to copy small data chunks.
 * For conditions of distribution and use, see copyright notice in zlib.h
 */
#ifndef MEMCOPY_H_
#define MEMCOPY_H_

/* Load 64 bits from IN and place the bytes at offset BITS in the result. */
static inline uint64_t load_64_bits(const unsigned char *in, unsigned bits) {
  uint64_t chunk;
  MEMCPY(&chunk, in, sizeof(chunk));

#if BYTE_ORDER == LITTLE_ENDIAN
  return chunk << bits;
#else
  return ZSWAP64(chunk) << bits;
#endif
}

static inline unsigned char *copy_1_bytes(uint8_t *out, uint8_t *from) {
    *out++ = *from;
    return out;
}

static inline unsigned char *copy_2_bytes(uint8_t *out, uint8_t *from) {
    uint16_t chunk;
    uint32_t sz = sizeof(chunk);
    MEMCPY(&chunk, from, sz);
    MEMCPY(out, &chunk, sz);
    return out + sz;
}

static inline uint8_t *copy_3_bytes(uint8_t *out, uint8_t *from) {
    out = copy_1_bytes(out, from);
    return copy_2_bytes(out, from + 1);
}

static inline uint8_t *copy_4_bytes(uint8_t *out, uint8_t *from) {
    uint32_t chunk;
    uint32_t sz = sizeof(chunk);
    MEMCPY(&chunk, from, sz);
    MEMCPY(out, &chunk, sz);
    return out + sz;
}

static inline uint8_t *copy_5_bytes(uint8_t *out, uint8_t *from) {
    out = copy_1_bytes(out, from);
    return copy_4_bytes(out, from + 1);
}

static inline uint8_t *copy_6_bytes(uint8_t *out, uint8_t *from) {
    out = copy_2_bytes(out, from);
    return copy_4_bytes(out, from + 2);
}

static inline uint8_t *copy_7_bytes(uint8_t *out, uint8_t *from) {
    out = copy_3_bytes(out, from);
    return copy_4_bytes(out, from + 3);
}

static inline uint8_t *copy_8_bytes(uint8_t *out, uint8_t *from) {
    uint64_t chunk;
    uint32_t sz = sizeof(chunk);
    MEMCPY(&chunk, from, sz);
    MEMCPY(out, &chunk, sz);
    return out + sz;
}

/* Copy LEN bytes (7 or fewer) from FROM into OUT. Return OUT + LEN. */
static inline uint8_t *copy_bytes(uint8_t *out, uint8_t *from, uint32_t len) {
    Assert(len < 8, "copy_bytes should be called with less than 8 bytes");

#ifndef UNALIGNED_OK
    while (len--) {
        *out++ = *from++;
    }
    return out;
#else
    switch (len) {
    case 7:
        return copy_7_bytes(out, from);
    case 6:
        return copy_6_bytes(out, from);
    case 5:
        return copy_5_bytes(out, from);
    case 4:
        return copy_4_bytes(out, from);
    case 3:
        return copy_3_bytes(out, from);
    case 2:
        return copy_2_bytes(out, from);
    case 1:
        return copy_1_bytes(out, from);
    case 0:
        return out;
    default:
        Assert(0, "should not happen");
    }

    return out;
#endif /* UNALIGNED_OK */
}

/* Copy LEN bytes (7 or fewer) from FROM into OUT. Return OUT + LEN. */
static inline uint8_t *set_bytes(uint8_t *out, uint8_t *from, uint32_t dist, uint32_t len) {
    Assert(len < 8, "set_bytes should be called with less than 8 bytes");

#ifndef UNALIGNED_OK
    while (len--) {
        *out++ = *from++;
    }
    return out;
#else
    if (dist >= len)
        return copy_bytes(out, from, len);

    switch (dist) {
    case 6:
        Assert(len == 7, "len should be exactly 7");
        out = copy_6_bytes(out, from);
        return copy_1_bytes(out, from);

    case 5:
        Assert(len == 6 || len == 7, "len should be either 6 or 7");
        out = copy_5_bytes(out, from);
        return copy_bytes(out, from, len - 5);

    case 4:
        Assert(len == 5 || len == 6 || len == 7, "len should be either 5, 6, or 7");
        out = copy_4_bytes(out, from);
        return copy_bytes(out, from, len - 4);

    case 3:
        Assert(4 <= len && len <= 7, "len should be between 4 and 7");
        out = copy_3_bytes(out, from);
        switch (len) {
        case 7:
            return copy_4_bytes(out, from);
        case 6:
            return copy_3_bytes(out, from);
        case 5:
            return copy_2_bytes(out, from);
        case 4:
            return copy_1_bytes(out, from);
        default:
            Assert(0, "should not happen");
            break;
        }

    case 2:
        Assert(3 <= len && len <= 7, "len should be between 3 and 7");
        out = copy_2_bytes(out, from);
        switch (len) {
        case 7:
            out = copy_4_bytes(out, from);
            out = copy_1_bytes(out, from);
            return out;
        case 6:
            out = copy_4_bytes(out, from);
            return out;
        case 5:
            out = copy_2_bytes(out, from);
            out = copy_1_bytes(out, from);
            return out;
        case 4:
            out = copy_2_bytes(out, from);
            return out;
        case 3:
            out = copy_1_bytes(out, from);
            return out;
        default:
            Assert(0, "should not happen");
            break;
        }

    case 1:
        Assert(2 <= len && len <= 7, "len should be between 2 and 7");
        uint8_t c = *from;
        switch (len) {
        case 7:
            MEMSET(out, c, 7);
            return out + 7;
        case 6:
            MEMSET(out, c, 6);
            return out + 6;
        case 5:
            MEMSET(out, c, 5);
            return out + 5;
        case 4:
            MEMSET(out, c, 4);
            return out + 4;
        case 3:
            MEMSET(out, c, 3);
            return out + 3;
        case 2:
            MEMSET(out, c, 2);
            return out + 2;
        default:
            Assert(0, "should not happen");
            break;
        }
    }
    return out;
#endif /* UNALIGNED_OK */
}

/* Byte by byte semantics: copy LEN bytes from OUT + DIST and write them to OUT. Return OUT + LEN. */
static inline uint8_t *chunk_memcpy(uint8_t *out, uint8_t *from, uint32_t len) {
    uint32_t sz = sizeof(uint64_t);
    Assert(len >= sz, "chunk_memcpy should be called on larger chunks");

    /* Copy a few bytes to make sure the loop below has a multiple of SZ bytes to be copied. */
    copy_8_bytes(out, from);

    uint32_t rem = len % sz;
    len /= sz;
    out += rem;
    from += rem;

    uint32_t by8 = len % sz;
    len -= by8;
    switch (by8) {
    case 7:
        out = copy_8_bytes(out, from);
        from += sz;
    case 6:
        out = copy_8_bytes(out, from);
        from += sz;
    case 5:
        out = copy_8_bytes(out, from);
        from += sz;
    case 4:
        out = copy_8_bytes(out, from);
        from += sz;
    case 3:
        out = copy_8_bytes(out, from);
        from += sz;
    case 2:
        out = copy_8_bytes(out, from);
        from += sz;
    case 1:
        out = copy_8_bytes(out, from);
        from += sz;
    }

    while (len) {
        out = copy_8_bytes(out, from);
        from += sz;
        out = copy_8_bytes(out, from);
        from += sz;
        out = copy_8_bytes(out, from);
        from += sz;
        out = copy_8_bytes(out, from);
        from += sz;
        out = copy_8_bytes(out, from);
        from += sz;
        out = copy_8_bytes(out, from);
        from += sz;
        out = copy_8_bytes(out, from);
        from += sz;
        out = copy_8_bytes(out, from);
        from += sz;

        len -= 8;
    }

    return out;
}

/* Memset LEN bytes in OUT with the value at OUT - 1. Return OUT + LEN. */
static inline uint8_t *byte_memset(uint8_t *out, uint32_t len) {
    uint32_t sz = sizeof(uint64_t);
    Assert(len >= sz, "byte_memset should be called on larger chunks");

    uint8_t *from = out - 1;
    uint8_t c = *from;

    /* First, deal with the case when LEN is not a multiple of SZ. */
    MEMSET(out, c, sz);
    uint32_t rem = len % sz;
    len /= sz;
    out += rem;

    uint32_t by8 = len % 8;
    len -= by8;
    switch (by8) {
    case 7:
        MEMSET(out, c, sz);
        out += sz;
    case 6:
        MEMSET(out, c, sz);
        out += sz;
    case 5:
        MEMSET(out, c, sz);
        out += sz;
    case 4:
        MEMSET(out, c, sz);
        out += sz;
    case 3:
        MEMSET(out, c, sz);
        out += sz;
    case 2:
        MEMSET(out, c, sz);
        out += sz;
    case 1:
        MEMSET(out, c, sz);
        out += sz;
    }

    while (len) {
        /* When sz is a constant, the compiler replaces __builtin_memset with an
           inline version that does not incur a function call overhead. */
        MEMSET(out, c, sz);
        out += sz;
        MEMSET(out, c, sz);
        out += sz;
        MEMSET(out, c, sz);
        out += sz;
        MEMSET(out, c, sz);
        out += sz;
        MEMSET(out, c, sz);
        out += sz;
        MEMSET(out, c, sz);
        out += sz;
        MEMSET(out, c, sz);
        out += sz;
        MEMSET(out, c, sz);
        out += sz;
        len -= 8;
    }

    return out;
}

/* Copy DIST bytes from OUT - DIST into OUT + DIST * k, for 0 <= k < LEN/DIST. Return OUT + LEN. */
static inline uint8_t *chunk_memset(uint8_t *out, uint8_t *from, uint32_t dist, uint32_t len) {
    if (dist >= len)
        return chunk_memcpy(out, from, len);

    Assert(len >= sizeof(uint64_t), "chunk_memset should be called on larger chunks");

    /* Double up the size of the memset pattern until reaching the largest pattern of size less than SZ. */
    uint32_t sz = sizeof(uint64_t);
    while (dist < len && dist < sz) {
        copy_8_bytes(out, from);

        out += dist;
        len -= dist;
        dist += dist;

        /* Make sure the next memcpy has at least SZ bytes to be copied.  */
        if (len < sz)
            /* Finish up byte by byte when there are not enough bytes left. */
            return set_bytes(out, from, dist, len);
    }

    return chunk_memcpy(out, from, len);
}

/* Byte by byte semantics: copy LEN bytes from FROM and write them to OUT. Return OUT + LEN. */
static inline uint8_t *chunk_copy(uint8_t *out, uint8_t *from, int dist, uint32_t len) {
    if (len < sizeof(uint64_t)) {
        if (dist > 0)
            return set_bytes(out, from, dist, len);

        return copy_bytes(out, from, len);
    }

    if (dist == 1)
        return byte_memset(out, len);

    if (dist > 0)
        return chunk_memset(out, from, dist, len);

    return chunk_memcpy(out, from, len);
}

#endif /* MEMCOPY_H_ */
