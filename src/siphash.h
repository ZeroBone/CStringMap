#ifndef CSTRINGMAP_SIPHASH_H
#define CSTRINGMAP_SIPHASH_H

#include <stdint.h>
#include <stdlib.h>

uint32_t halfsiphash(const uint8_t*, const size_t, const uint64_t);

#endif //CSTRINGMAP_SIPHASH_H