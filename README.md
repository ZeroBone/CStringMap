# CStringMap
Header-only, optimized Hash Map implementation based on Robin Hood hashing implemented in C99.
# Features
* Header-only.
* Open-addressed, based on very efficient Robin Hood hashing algorithm.
* Fast and very memory efficient internal string representation.
* No dynamic memory allocation is made for short keys.
* DOS/DDOS-safe default hashing algorithm.
* The hashing algorithm can be easily changed (by modifying the `stringhm_hash` macro).
* Arbitrary values (void*).
* Interators for convenient hash map traversing.
* Adjustable collapse/expand load factor.
* The minimum capacity can be specified at initialization. The hash map will never get smaller than this value.
* No destriptor heap-binding: you can allocate the hash table destriptor anywhere.
* Readable and documented code.
* Memory-safe: All memory errors are carefully handled and reported to the calee.
  This allows using this hash map implementation in robust software and on embedded platforms.   
# Limitations
* Only string keys are supported. (But it is easy to modify the code to handle arbitrary keys).
* This data structure has an internal string implementation that is designed for fast string comparison and low memory usage.
  When a new key is added the c string and it's length is converted to the internal string representation.
  Therefore, all the keys are copied. This is normally what you want as a developer, but this could be a limitaion in some rare cases.
* The maximum amount of elements in a hash map is `UINT_MAX / 2`. 
# Usage
All the files from the `src` directory can be dropped into an existing C/C++ project and compiled along with it.
## API
**Note**: for performance reasons the length of the key should be passed to the functions together with a `char*` pointer.
This is because the length is often known in the context of the calee and there is no need to recalculate it again.
If you don't know a string's length, call `strlen()` from `string.h` to calculate it.
Invalid length will cause undefined behaviour.
### `bool stringhm_init(stringhm_t* hm, size_t initialCapacity)`
Initializes the hash map descriptor with the initial (and minimum) capacity `initialCapacity`.

Returns `false` if there was a memory error during initialization,

Returns `true` if the hashmap was initialized successfully.

**Important**: if this function returned `false`, no guarantees are given on the hm memory.
Any other function call including `stringhm_destroy()` will lead to undefined behaviour.

Allocation on the stack:
```c
stringhm_t hm;
if (!stringhm_init(&hm, STRINGHM_INITIAL_CAPACITY_DEFAULT)) {
    // error handling, hm is garbage
}
else {
    // we can use the hash map now, don't forget to call:
    stringhm_destroy(&hm);
}
```
Allocation on the heap:
```c
stringhm_t* hm = (stringhm_t*)malloc(sizeof(stringhm_t));
if (hm == NULL) {
    // error handling
}
else if (!stringhm_init(hm, STRINGHM_INITIAL_CAPACITY_DEFAULT)) {
    free(hm);
    // error handling
}
else {
    // hashmap usage
    stringhm_destroy(hm);
    free(hm);
}
```

### `void* stringhm_find(const stringhm_t* hm, const char* key, size_t keyLength)`
Searches the key in the hash map.

Returns pointer to the found value or `NULL` if there is no such key.

Worst case complexity: `O(n)` Average: `O(1)`

### `void* stringhm_add(stringhm_t* hm, const char* key, size_t keyLength, void* value)`
Adds a key-value pair to the hash map.

Returns:
* `NULL` on success.
* pointer to the duplicating element's payload if such key already exists.
* `hm` on memory error.

Worst case complexity: `O(n)` Average: `O(1)`

### `void* stringhm_remove(stringhm_t* hm, const char* key, size_t keyLength)`
Removes a key-value from the hash map by the key.

Returns:
* pointer to the deleted element's value (this allows you to free value's memory if needed).
* `NULL` if there is no element with such key.
* `hm` on memory error.

Worst case complexity: `O(n)` Average: `O(1)`

### `void stringhm_destroy(stringhm_t* hm)`
Destroyes the hash map without touching the values.

This function assumes you have already handled hash map's value pointers memory.

If you need to destroy the values separately, use the `stringhm_destroyWithValues()` function.
### `void stringhm_destroyWithValues(stringhm_t* hm, void (*destructor)(void*))`
Destroyes the hash map's values along with the hash map itself.

This function accepts a destructor function that frees the memory.

If the values in your hash map are poiners to dynamic memory but
aren't complex structs themselves with dynamic memory pointers inside,
you can just pass `free`:

```c
stringhm_destroyWithValues(&hm, free);
```

### Error handling
If some internal error (e.g. memory error) in some function's code except `stringhm_init` occurres, the state of the hash map is not corrupted.
In other words, the hash map doesn't destroy itself when some error is encountered.  

If some function reported an error, it only means that this action was rolled back.
So you can still use the API functions. 

If an error is reported and you wish to destroy the hash map, don't forget to destroy your values and call `stringhm_destroy()`.
You can use the `stringhm_destroyWithValues()` function to destroy values along with destroying the hash map. 
# License
Copyright (c) 2019 Alexander Mayorov.

This project is licensed under the MIT License.

Please be kind and leave a copyright notice if you use or modify this software.

For more details see the LICENCE file.