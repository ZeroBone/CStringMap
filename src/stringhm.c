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

#include <stdlib.h>
#include <inttypes.h>
#include <limits.h>
#include <string.h>
#include <assert.h>
#include "siphash.h"
#include "stringhm.h"

#define EXPAND_LIMIT (1024U * 1024)

// true if length > ~85% * capacity (885 / 1024)
#define THRESHOLD_HIGH(length, capacity) ((length) > (((capacity) * 885U) >> 10U))
// #define THRESHOLD_HIGH(length, capacity) ((length) > ((float)(capacity)) * 85.0f)
// true if length < ~40% * capacity (403 / 1024)
#define THRESHOLD_LOW(length, capacity) ((length) < (((capacity) * 403U) >> 10U))
// #define THRESHOLD_LOW(length, capacity) ((length) < ((float)(capacity)) * 40.0f)

#if _WIN32 || _WIN64
#if _WIN64
#define PTR64
#else
#define PTR32
#endif
#elif __GNUC__
#if __x86_64__ || __ppc64__
        #define PTR64
    #else
        #define PTR32
    #endif
#elif UINTPTR_MAX > UINT_MAX
    #define PTR64
#else
    #define PTR32
#endif

#ifdef PTR64
#define STRING_CACHED_BUFFER_SIZE 8
#else
#define STRING_CACHED_BUFFER_SIZE 4
#endif

typedef struct {
    size_t length;
    union {
        char cachedBuffer[STRING_CACHED_BUFFER_SIZE];
        char* buffer;
    } payload;
} stringhm_string_t;

#define STRING_BUFFER(str) ((str).length < STRING_CACHED_BUFFER_SIZE ? (str).payload.cachedBuffer : (str).payload.buffer)

#define STRING_EQUALS(str1, str2) (\
    (str1).length == (str2).length && \
    (str1).length < STRING_CACHED_BUFFER_SIZE ? \
        !memcmp((str1).payload.cachedBuffer, (str2).payload.cachedBuffer, (str1).length) : \
        !memcmp((str1).payload.buffer, (str2).payload.buffer, (str1).length) \
    )

#define STRING_EQUALSC(str, cstr, cstrlen) (\
    (str).length == (cstrlen) && \
    (str).length < STRING_CACHED_BUFFER_SIZE ? \
        !memcmp((str).payload.cachedBuffer, cstr, cstrlen) : \
        !memcmp((str).payload.buffer, cstr, cstrlen) \
    )

#define STRING_DESTROY(str) \
    if ((str).length >= STRING_CACHED_BUFFER_SIZE) { \
        free((str).payload.buffer); \
    }

typedef struct stringhm_element_s {
    stringhm_string_t key;
    uint32_t hash;
    uint16_t probeSequenceLength;
    void* payload;
} stringhm_element_t;

#define stringhm_hash(hm, key, keyLength) halfsiphash((unsigned char*)key, keyLength, hm->seed)

#define EMPTY_ELEMENT(b) ((b).key.length = 0)
#define ELEMENT_IS_EMPTY(b) ((b).key.length == 0)
#define ELEMENT_IS_NOT_EMPTY(b) ((b).key.length != 0)

#define assert_probeSequenceLength(hm, el, i) \
    { \
        const size_t startIndex = el->hash % hm->capacity; \
        const size_t distance = (startIndex > i) ? hm->capacity - startIndex + i : i - startIndex; \
        const bool check = ELEMENT_IS_EMPTY(*el) || distance == el->probeSequenceLength; \
        assert(check); \
    }

/**
 * Inserts the value in the hashmap.
 * This function doesn't check that there is enouph memory allocated in the table.
 * It assumes the hashmap has already been resized if needed.
 * That's why this function is not exposed to the user and is only used internally.
 * @param hm the hashmap to insert to.
 * @param key the key.
 * @param value the value.
 * @return NULL on success, pointer to the dublicating value on error
 * @private
 */
void* stringhm_insert(stringhm_t* const hm, const stringhm_string_t key, void* const value) {

    assert(hm != NULL);
    assert(key.length > 0);
    assert(value != NULL);

    const uint32_t hash = stringhm_hash(hm, STRING_BUFFER(key), key.length);

    stringhm_element_t* element;

    stringhm_element_t entry = {
        .key = key,
        .hash = hash,
        .probeSequenceLength = 0,
        .payload = value
    };

    // if the probe sequence length of the element being inserted is greater than PSL of the current element =>
    // => swap them and continue

    size_t i = hash % hm->capacity;

    for (;;) {

        element = &hm->table[i];

        if (ELEMENT_IS_NOT_EMPTY(*element)) {

            assert_probeSequenceLength(hm, element, i)

            // did we find the key?
            if (element->hash == hash && STRING_EQUALS(element->key, key)) {
                // dublicate key, insertion failed

                return element->payload;

            }

            // rich element
            if (entry.probeSequenceLength > element->probeSequenceLength) {

                stringhm_element_t temp;

                // swap the rich element with the current entry
                temp = entry;
                entry = *element;
                *element = temp;

            }

            entry.probeSequenceLength++;

            assert_probeSequenceLength(hm, element, i)

            i = (i + 1) % hm->capacity;

            continue;

        }

        break;

    }

    // insert the value
    *element = entry;

    hm->length++;

    assert_probeSequenceLength(hm, element, i);

    return NULL;

}

/**
 * Rehashes the hashmap: increases (or decreases) the capacity to newCapacity
 * @param hm the hash map.
 * @param newCapacity the new capacity.
 * @return true on error, false on success.
 */
bool stringhm_rehash(stringhm_t* const hm, const size_t newCapacity) {

    assert(hm != NULL);
    assert(newCapacity > 0);
    assert(newCapacity > hm->length);

    stringhm_element_t* oldTable = hm->table; // can be NULL
    const size_t oldCapacity = hm->capacity;

    stringhm_element_t* newTable;

    // overflow check
    if (newCapacity > UINT_MAX / 2) {
        return true;
    }

    // calloc instead of malloc because we need the memory to be initialized with zeroes
    // because if it is garbage memory, we cannot identify empty elements
    // Btw., calloc() is faster than malloc() and memset()
    newTable = calloc(newCapacity, sizeof(stringhm_element_t));

    if (newTable == NULL) {
        return true;
    }

    hm->table = newTable;
    hm->capacity = newCapacity;
    hm->length = 0;

    hm->seed ^= (unsigned int)rand(); // the first time we will xor the seed with garbage memory

    size_t i;
    for (i = 0; i < oldCapacity; i++) {

        const stringhm_element_t* element = &oldTable[i];

        // ignore empty elements
        if (ELEMENT_IS_EMPTY(*element)) {
            continue;
        }

        assert(stringhm_insert(hm, element->key, element->payload) == NULL);

        // we don't need to destroy the key here as we don't copy it in stringhm_insert

    }

    if (oldTable != NULL) {
        free(oldTable);
    }

    return false; // success

}

/**
 * Initializes the hashmap.
 * @param hm the hashmap struct to initialize.
 * @param initialCapacity the initial and mininum capacity of the hashmap. Use STRINGHM_DEFAULT_INITIAL_CAPACITY for the default value.
 * @return false on error, true on success
 */
bool stringhm_init(stringhm_t* const hm, const size_t initialCapacity) {

    assert(hm != NULL);

    hm->capacity = 0;
    hm->length = 0;

    hm->table = NULL;

    hm->minCapacity = initialCapacity;

    if (stringhm_rehash(hm, hm->minCapacity)) {
        return false;
    }

    assert(hm->table != NULL);
    assert(hm->capacity > 0);

    return true;

}

/**
 * Searches for the given key and returns the corresponding value.
 * @param hm the hashmap.
 * @param key the key. Empty strings are not allowed.
 * @param keyLength the length (in bytes) of the key. Should not be 0.
 * @return pointer in the hashmap to the found value or null if there is no such key.
 */
void* stringhm_find(const stringhm_t* const hm, const char* const key, const size_t keyLength) {

    assert(hm != NULL);
    assert(keyLength != 0);
    assert(key != NULL);

    const uint32_t hash = stringhm_hash(hm, (const uint8_t* const)key, keyLength);

    size_t i = hash % hm->capacity;
    size_t probeSequenceLength = 0;

    // start linear probing
    stringhm_element_t* currentElement;

    for (;;) {

        currentElement = &hm->table[i];

        // stop the linear probe if we encounter an empty element
        // or if we see an element with probe sequence length
        // lower than the current distance from the base location
        if (ELEMENT_IS_EMPTY(*currentElement) || probeSequenceLength > currentElement->probeSequenceLength) {
            return NULL;
        }

        assert_probeSequenceLength(hm, currentElement, i);

        if (currentElement->hash == hash && STRING_EQUALSC(currentElement->key, key, keyLength)) {

            assert(currentElement->payload != NULL);

            return currentElement->payload;

        }

        probeSequenceLength++;

        i = (i + 1) % hm->capacity;

    }

}

/**
 * Insert a key-value pair in the hashmap.
 * @param hm hash map.
 * @param key key.
 * @param keyLength key length.
 * @param value value.
 * @return NULL on success, pointer to the duplicating element's payload on error or hashmap pointer on memory error
 */
void* stringhm_add(stringhm_t* const hm, const char* key, const size_t keyLength, void* const value) {

    assert(hm != NULL);
    assert(keyLength > 0);
    assert(value != NULL);

    // check the load factor and rehash if it exceeds the threshold
    if (THRESHOLD_HIGH(hm->length, hm->capacity)) {
        // double the size of the hashmap but limit the step

        const size_t expandLimit = hm->capacity + EXPAND_LIMIT;

        const size_t newCapacity = (hm->capacity << 1U) < expandLimit ? hm->capacity << 1U : expandLimit; // minimum

        if (stringhm_rehash(hm, newCapacity)) {
            // memory error occurred
            return hm;
        }

    }

    stringhm_string_t internalKey = {
        .length = keyLength
    };

    if (keyLength < STRING_CACHED_BUFFER_SIZE) {
        memcpy(internalKey.payload.cachedBuffer, key, keyLength);
    }
    else {
        char* buffer = (char*)malloc(sizeof(char) * keyLength);

        if (buffer == NULL) {
            return hm;
        }

        memcpy(buffer, key, keyLength);

        internalKey.payload.buffer = buffer;

    }

    return stringhm_insert(hm, internalKey, value);

}

#undef EXPAND_LIMIT

/**
 * Removes they key-value pair from the hash map.
 * @param hm hash map.
 * @param key key.
 * @param keyLength length (in bytes) of the key.
 * @return null if the element was not found, hashmap pointer on memory error, associated payload pointer on success
 */
void* stringhm_remove(stringhm_t* const hm, const char* const key, const size_t keyLength) {

    assert(hm != NULL);
    assert(key != NULL);
    assert(keyLength > 0);

    const uint32_t hash = stringhm_hash(hm, (const uint8_t* const)key, keyLength);

    size_t i = hash % hm->capacity;
    size_t probeSequenceLength = 0;

    stringhm_element_t* element;

    for (;;) {

        element = &hm->table[i];

        if (ELEMENT_IS_EMPTY(*element) || probeSequenceLength > element->probeSequenceLength) {
            return NULL; // not found
        }

        assert_probeSequenceLength(hm, element, i);

        if (element->hash == hash && STRING_EQUALSC(element->key, key, keyLength)) {

            assert(element->payload != NULL);

            break;

        }

        probeSequenceLength++;

        i = (i + 1) % hm->capacity;

    }

    // destroy the key
    STRING_DESTROY(element->key);

    void* const payload = element->payload; // save the payload before it gets overwritten

    hm->length--;

    // backwards-shifting method
    for (;;) {

        EMPTY_ELEMENT(*element);

        i = (i + 1) % hm->capacity;

        stringhm_element_t* currentElement = &hm->table[i];

        assert_probeSequenceLength(hm, currentElement, i);

        // stop if either one of the two requirements are met:
        // 1. we reach an empty element
        // 2. the PSL is in it's original location.
        if (ELEMENT_IS_EMPTY(*currentElement) || currentElement->probeSequenceLength == 0) {
            break;
        }

        currentElement->probeSequenceLength--;

        *element = *currentElement; // perform the shift
        element = currentElement; // shift the pointers for the next iteration

    }

    // maybe we need to shrink the hash map
    if (hm->length > hm->minCapacity && THRESHOLD_LOW(hm->length, hm->capacity)) {

        // half the size but not exceed the limit (the limit is the initial (min) capacity)
        if (stringhm_rehash(hm, hm->capacity >> 1U > hm->minCapacity ? hm->capacity >> 1U : hm->minCapacity)) {
            return hm; // memory error
        }

    }

    return payload;

}

stringhm_iterator_t stringhm_iteratorCreate() {
    return 0;
}

stringhm_iterator_t stringhm_iteratorNextKey(const stringhm_t* const hm, stringhm_iterator_t iterator, const char** const key) {

    while (iterator < hm->capacity) {

        stringhm_element_t* currentElement = &hm->table[iterator];

        iterator++;

        if (ELEMENT_IS_NOT_EMPTY(*currentElement)) {
            *key = STRING_BUFFER(currentElement->key);
            return iterator;
        }

    }

    return 0;

}

stringhm_iterator_t stringhm_iteratorNextValue(const stringhm_t* const hm, stringhm_iterator_t iterator, void** const value) {

    while (iterator < hm->capacity) {

        stringhm_element_t* currentElement = &hm->table[iterator];

        iterator++;

        if (ELEMENT_IS_NOT_EMPTY(*currentElement)) {
            *value = currentElement->payload;
            return iterator;
        }

    }

    return 0;

}

stringhm_iterator_t stringhm_iteratorNextKeyValue(const stringhm_t* const hm, stringhm_iterator_t iterator, const char** const key, void** const value) {

    while (iterator < hm->capacity) {

        stringhm_element_t* currentElement = &hm->table[iterator];

        iterator++;

        if (ELEMENT_IS_NOT_EMPTY(*currentElement)) {
            *key = STRING_BUFFER(currentElement->key);
            *value = currentElement->payload;
            return iterator;
        }

    }

    return 0;

}

/**
 * Destroyes the hash map.
 * If the hash map is allocated on the heap, it is the caller's
 * responsibility to free the memory for the descriptor.
 * Here we don't call free because we don't know whether the struct is allocated on the stack or on the heap.
 * It is also the responsibility of the caller to free the payload if it points to some heap-allocated block.
 * @param hm the hash map to destroy.
 */
void stringhm_destroy(stringhm_t* const hm) {

    stringhm_element_t* currentElement;
    size_t i;

    for (i = 0; i < hm->capacity; i++) {

        currentElement = &hm->table[i];

        if (ELEMENT_IS_NOT_EMPTY(*currentElement)) {
            STRING_DESTROY(currentElement->key);
        }

    }

    free(hm->table);

}

void stringhm_destroyWithValues(stringhm_t* const hm, void (*destructor)(void*)) {

    stringhm_element_t* currentElement;
    size_t i;

    for (i = 0; i < hm->capacity; i++) {

        currentElement = &hm->table[i];

        if (ELEMENT_IS_NOT_EMPTY(*currentElement)) {
            STRING_DESTROY(currentElement->key);
            destructor(currentElement->payload);
        }

    }

    free(hm->table);

}

#undef THRESHOLD_HIGH
#undef THRESHOLD_LOW

#undef EMPTY_ELEMENT
#undef ELEMENT_IS_EMPTY
#undef ELEMENT_IS_NOT_EMPTY
#undef assert_probeSequenceLength