/**
 * Copyright (c) 2019 Alexander Mayorov <zerobone21@gmail.com>
 *
 * This software is licenced under the MIT Licence.
 * If you are using this software or modifying it, please leave a copyright notice.
 * For more information see the LICENCE file.
 *
 * Repository:
 * https://github.com/ZeroBone/CStringMap
 */

#ifndef CSTRINGMAP_STRINGHM_H
#define CSTRINGMAP_STRINGHM_H

#include <stdbool.h>
#include <stdint.h>

#define STRINGHM_INITIAL_CAPACITY_DEFAULT 16

struct stringhm_element_s;

typedef struct {
    size_t capacity;
    size_t length;
    struct stringhm_element_s* table;
    uint64_t seed;
    size_t minCapacity;
} stringhm_t;

typedef size_t stringhm_iterator_t;

bool stringhm_init(stringhm_t* const hm, const size_t initialCapacity);
void* stringhm_find(const stringhm_t* const hm, const char* const key, const size_t keyLength);
void* stringhm_add(stringhm_t* const hm, const char* key, const size_t keyLength, void* const value);
void* stringhm_remove(stringhm_t* const hm, const char* const key, const size_t keyLength);
// iterator
stringhm_iterator_t stringhm_iteratorCreate();
stringhm_iterator_t stringhm_iteratorNextKey(const stringhm_t* const hm, stringhm_iterator_t iterator, const char** const key);
stringhm_iterator_t stringhm_iteratorNextValue(const stringhm_t* const hm, stringhm_iterator_t iterator, void** const value);
stringhm_iterator_t stringhm_iteratorNextKeyValue(const stringhm_t* const hm, stringhm_iterator_t iterator, const char** const key, void** const value);
// destructor
void stringhm_destroy(stringhm_t* const hm);
void stringhm_destroyWithValues(stringhm_t* const hm, void (*destructor)(void*));

#endif //CSTRINGMAP_STRINGHM_H