#include "benchmark.h"

#include "timer.h"
#include "../src/stringhm.h"

double benchmarkAdd(const size_t times) {

    stringhm_t hm;
    stringhm_init(&hm, STRINGHM_INITIAL_CAPACITY_DEFAULT);

    size_t i = 0;
    char str[16];

    double start = getTime();

    for (; i < times; i++) {

        double breakTime = getTime();
        snprintf(str, 16, "%llx", i);
        start += getTime() - breakTime;

        stringhm_add(&hm, str, 4, &start);

    }

    double result = getTime() - start;

    stringhm_destroy(&hm);

    return result;

}

double benchmarkFind(const size_t times) {

    stringhm_t hm;
    stringhm_init(&hm, STRINGHM_INITIAL_CAPACITY_DEFAULT);

    size_t i = 0;
    char str[16];

    for (; i < times; i++) {

        snprintf(str, 16, "%llx", i);

        stringhm_add(&hm, str, 4, &i);

    }

    i = 0;

    double start = getTime();

    for (; i < times; i++) {

        double breakTime = getTime();
        snprintf(str, 16, "%llx", i);
        start += getTime() - breakTime;

        stringhm_find(&hm, str, 4);

    }

    double result = getTime() - start;

    stringhm_destroy(&hm);

    return result;

}

double benchmarkDelete(const size_t times) {

    stringhm_t hm;
    stringhm_init(&hm, STRINGHM_INITIAL_CAPACITY_DEFAULT);

    size_t i = 0;
    char str[16];

    for (; i < times; i++) {

        snprintf(str, 16, "%llx", i);

        stringhm_add(&hm, str, 4, &i);

    }

    i = 0;

    double start = getTime();

    for (; i < times; i++) {

        double breakTime = getTime();
        snprintf(str, 16, "%llx", i);
        start += getTime() - breakTime;

        stringhm_remove(&hm, str, 4);

    }

    double result = getTime() - start;

    printf("Number of elements after deletion of entire hash map: %zu (should be zero)\n", hm.length);
    printf("Capacity after deletion of entire hash map: %zu\n", hm.capacity);

    stringhm_destroy(&hm);

    return result;

}

void runBenchmark(const char* const name, double (*b)(size_t)) {

    printf("======[ %s ]======\n", name);

    size_t times;
    double result;
    for (times = 1000; times <= 100000000U; times *= 10U) {

        result = b(times);

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

    // runBenchmark("ADD", benchmarkAdd);

    // runBenchmark("FIND", benchmarkFind);

    runBenchmark("DELETE", benchmarkDelete);

}