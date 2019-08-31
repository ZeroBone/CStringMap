/*
 * SipHash reference C implementation
 *
 * Copyright (c) 2016 Jean-Philippe Aumasson <jeanphilippe.aumasson@gmail.com>
 *
 * To the extent possible under law, the author(s) have dedicated all copyright
 * and related and neighboring rights to this software to the public domain
 * worldwide. This software is distributed without any warranty.
 *
 * You should have received a copy of the CC0 Public Domain Dedication along
 * with this software. If not, see
 * <http://creativecommons.org/publicdomain/zero/1.0/>.
 *
 * The implementation was modified and improved by Alexander Mayorov.
 * Copyright (c) 2019 Alexander Mayorov <zerobone21@gmail.com>
 * The modified implementation is Licenced under the MIT Licence.
 * For more information see the LICENCE file.
 */

#include "siphash.h"

#define cROUNDS 2
#define dROUNDS 4

#define ROTL(x, b)    (uint32_t)(((x) << (b)) | ((x) >> (32U - (b))))

#define U32TO8_LE(p, v)            \
    (p)[0] = (uint8_t)((v));    \
    (p)[1] = (uint8_t)((v) >> 8U);    \
    (p)[2] = (uint8_t)((v) >> 16U);    \
    (p)[3] = (uint8_t)((v) >> 24U)

#define U8TO32_LE(p)                            \
    (((uint32_t)((p)[0])) | ((uint32_t)((p)[1]) << 8U) |        \
     ((uint32_t)((p)[2]) << 16U) | ((uint32_t)((p)[3]) << 24U))

#define SIPROUND            \
    do {                \
        v0 += v1;        \
        v1 = ROTL(v1, 5U);    \
        v1 ^= v0;        \
        v0 = ROTL(v0, 16U);    \
        v2 += v3;        \
        v3 = ROTL(v3, 8U);    \
        v3 ^= v2;        \
        v0 += v3;        \
        v3 = ROTL(v3, 7U);    \
        v3 ^= v0;        \
        v2 += v1;        \
        v1 = ROTL(v1, 13U);    \
        v1 ^= v2;        \
        v2 = ROTL(v2, 16U);    \
    } while (0)

uint32_t halfsiphash(const uint8_t* payload, const size_t payloadLength, const uint64_t key) {

    const uint8_t* end = payload + payloadLength - (payloadLength % sizeof(uint32_t));
    const unsigned left = payloadLength & 3U;

    uint32_t v0 = 0;
    uint32_t v1 = 0;
    uint32_t v2 = 0x6c796765;
    uint32_t v3 = 0x74656462;
    uint32_t k0 = (uint32_t)key;
    uint32_t k1 = (key >> 32U);
    uint32_t m;

    uint32_t b = ((uint32_t) payloadLength) << 24U;

    v3 ^= k1;
    v2 ^= k0;
    v1 ^= k1;
    v0 ^= k0;

    for (; payload != end; payload += 4) {
        m = U8TO32_LE(payload);
        v3 ^= m;
        for (unsigned i = 0; i < cROUNDS; ++i) {
            SIPROUND;
        }
        v0 ^= m;
    }

    switch (left) {
        case 3:
            b |= ((uint32_t) payload[2]) << 16U;
        case 2:
            b |= ((uint32_t) payload[1]) << 8U;
        case 1:
            b |= ((uint32_t) payload[0]);
            break;
        case 0:
            break;
        default:
            break;
    }

    v3 ^= b;

    for (unsigned i = 0; i < cROUNDS; ++i) {
        SIPROUND;
    }

    v0 ^= b;
    v2 ^= 0xffu;

    for (unsigned i = 0; i < dROUNDS; ++i) {
        SIPROUND;
    }

    b = v1 ^ v3;

    U32TO8_LE((uint8_t*) &m, b);

    return m;

}