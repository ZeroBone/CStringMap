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

#include <stdio.h>

#include "src/stringhm.h"

char* testInitDestroy() {

    stringhm_t hm;
    stringhm_init(&hm, STRINGHM_INITIAL_CAPACITY_DEFAULT);

    if (hm.length != 0) {
        return "Length should be 0";
    }

    stringhm_destroy(&hm);

    return NULL;

}

char* testAdd1() {

    int a = 5, b = 2;

    stringhm_t hm;
    stringhm_init(&hm, STRINGHM_INITIAL_CAPACITY_DEFAULT);

    if (stringhm_add(&hm, "abc", 3, &a) != NULL) {
        return "Error adding 'abc'";
    }

    if (hm.length != 1) {
        return "Length should be 1";
    }

    if (stringhm_add(&hm, "The C programming Language.", 27, &b) != NULL) {
        return "Error adding second string.";
    }

    if (stringhm_find(&hm, "something unknown", 17) != NULL) {
        return "Found non-existing key-value pair.";
    }

    if (stringhm_find(&hm, "abc", 3) != &a) {
        return "'abc' key corresponds to wrong value or was not found.";
    }

    if (stringhm_find(&hm, "The C programming Language.", 27) != &b) {
        return "Second key corresponds to wrong value or was not found.";
    }

    stringhm_destroy(&hm);

    return NULL;

}

char* testRemove1() {

    int a = 5, b = 2;

    stringhm_t hm;
    stringhm_init(&hm, STRINGHM_INITIAL_CAPACITY_DEFAULT);

    if (hm.length != 0) {
        return "Length should be 0 at initialization.";
    }

    if (stringhm_add(&hm, "abc", 3, &a) != NULL) {
        return "Error adding 'abc'";
    }

    if (hm.length != 1) {
        return "Length should be 1";
    }

    if (stringhm_find(&hm, "abc", 3) != &a) {
        return "'abc' key corresponds to wrong value or was not found.";
    }

    if (stringhm_remove(&hm, "abc", 3) != &a) {
        return "Remove should return the associated value.";
    }

    if (hm.length != 0) {
        return "Length should be 0 after removing.";
    }

    stringhm_destroy(&hm);

    return NULL;

}

int main() {

    char* (*tests[])() = {
        testInitDestroy,
        testAdd1,
        testRemove1,
        NULL
    };

    printf("sizeof(void*) => %zu\n", sizeof(void*));
    printf("sizeof(size_t) => %zu\n", sizeof(size_t));
    printf("sizeof(stringhm_t) => %zu\n", sizeof(stringhm_t));
    puts("Starting tests...");

    size_t testsFailed = 0;
    size_t testsPassed = 0;

    char* (**currentTest)() = tests;
    char* currentTestResponse;
    while (*currentTest != NULL) {

        printf("Running test %zu...\n", currentTest - tests);

        if ((currentTestResponse = (*currentTest)()) == NULL) {
            testsPassed++;
        }
        else {

            if (testsFailed == 0) {
                fputs("======[ FAIL ]======\n", stderr);
            }

            testsFailed++;

            fprintf(stderr, "Error: Test #%zu failed:\n", currentTest - tests);

            fprintf(stderr, "%s\n", currentTestResponse);

            fputs("===\n", stderr);
        }

        currentTest++;

    }

    if (testsFailed == 0) {
        puts("======[ PASS ]======");
        puts("All tests successfully passed.");
        return 0;
    }
    else {
        fputs("======[ FAIL ]======\n", stderr);
    }

    return 0;
}