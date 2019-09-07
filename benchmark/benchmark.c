#include "benchmark.h"

#include "timer.h"
#include "../src/stringhm.h"
#include <string.h>

static char benchmarkKey[256];
static size_t benchmarkKeyLength;

inline static void benchmarkGenerateKey(const size_t i) {

    snprintf(benchmarkKey, 255, "%zx", i);

    benchmarkKeyLength = strlen(benchmarkKey);

}

double benchmarkAdd(const size_t times) {

    stringhm_t hm;
    stringhm_init(&hm, STRINGHM_INITIAL_CAPACITY_DEFAULT);

    size_t i;

    double start = getTime();

    for (i = 0; i < times; i++) {

        double breakTime = getTime();
        benchmarkGenerateKey(i);
        start += getTime() - breakTime;

        stringhm_add(&hm, benchmarkKey, benchmarkKeyLength, &start);

    }

    double result = getTime() - start;

    stringhm_destroy(&hm);

    return result;

}

double benchmarkFind(const size_t times) {

    stringhm_t hm;
    stringhm_init(&hm, STRINGHM_INITIAL_CAPACITY_DEFAULT);

    size_t i;

    for (i = 0; i < times; i++) {

        benchmarkGenerateKey(i);

        stringhm_add(&hm, benchmarkKey, benchmarkKeyLength, &i);

    }

    double start = getTime();

    for (i = 0; i < times; i++) {

        double breakTime = getTime();
        benchmarkGenerateKey(i);
        start += getTime() - breakTime;

        if (stringhm_find(&hm, benchmarkKey, benchmarkKeyLength) == NULL) {
            printf("Could not find key '%s' with n = %zu\n", benchmarkKey, times);
            return -1;
        }

    }

    double result = getTime() - start;

    stringhm_destroy(&hm);

    return result;

}

double benchmarkDelete(const size_t times) {

    stringhm_t hm;
    stringhm_init(&hm, STRINGHM_INITIAL_CAPACITY_DEFAULT);

    size_t i;

    for (i = 0; i < times; i++) {

        benchmarkGenerateKey(i);

        // printf("Adding '%s' key... Length of hashmap: %10zu\n", str, hm.length);

        void* result = stringhm_add(&hm, benchmarkKey, benchmarkKeyLength, &i);

        if (result == &hm) {
            puts("Memory error while filling the hash map.");
            return -1;
        }
        else if (result != NULL) {
            puts("Duplicate element.");
            return -1;
        }

    }

    printf("Added total of %zu elements to the hash map.\n", hm.length);

    if (hm.length != times) {
        return -1;
    }

    double start = getTime();
    void* removeResult;

    for (i = 0; i < times; i++) {

        double breakTime = getTime();
        benchmarkGenerateKey(i);
        start += getTime() - breakTime;

        removeResult = stringhm_remove(&hm, benchmarkKey, benchmarkKeyLength);

        if (removeResult == NULL) {
            puts("ERROR removing element from hash map: element not found.");
            return -1;
        }
        else if (removeResult == &hm) {
            puts("ERROR removing element from hash map: memory error.");
            return -1;
        }

    }

    double result = getTime() - start;

    printf("Number of elements after deletion of entire hash map: %zu (should be zero)\n", hm.length);

    if (hm.length != 0) {
        return -1;
    }

    printf("Capacity after deletion of entire hash map: %zu\n", hm.capacity);

    stringhm_destroy(&hm);

    return result;

}

void runBenchmark(const char* const name, double (*b)(size_t), const size_t start, const size_t stop, const size_t mulStep) {

    printf("======[ %s ]======\n", name);

    size_t times;
    double result;
    for (times = start; times <= stop; times *= mulStep) {

        result = b(times);

        if (result < 0) {

            printf("[%s]: Benchmark failed with n = %-10zu!\n", name, times);

            break;

        }

        printf("[%s]: n = %-10zu Time: %10zu ms (%10zu us). 1 operation: %10zu ns\n",
            name, times,
            (size_t)(result * 1e3), (size_t)(result * 1e6),
            (size_t)(result / times * 1e9)
        );

    }

    printf("======[ END %s ]======\n", name);

}

void runBenchmarks() {

    puts("Starting benchmarks...");

    runBenchmark("ADD", benchmarkAdd, BENCHMARK_START, BENCHMARK_STOP, BENCHMARK_STEP);

    runBenchmark("FIND", benchmarkFind, BENCHMARK_START, BENCHMARK_STOP, BENCHMARK_STEP);

    runBenchmark("DELETE", benchmarkDelete, BENCHMARK_START, BENCHMARK_STOP, BENCHMARK_STEP);

}