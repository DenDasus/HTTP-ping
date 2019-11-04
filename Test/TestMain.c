#include "CuTest.h"
#include <stdio.h>
#include <limits.h>

#include "../utils.h"

void AvgCalc_smallValues(CuTest* tc) {
    unsigned short values[] = {10, 20, 30, 40, 50};

    unsigned short avg = AvgCalc(values, sizeof(values) / sizeof(unsigned short));

    CuAssertTrue(tc, avg == 30);
}

void AvgCalc_overflow(CuTest* tc) {
    unsigned short values[] = {10000, 20000, 30000, 40000, 50000};

    unsigned short avg = AvgCalc(values, sizeof(values) / sizeof(unsigned short));

    CuAssertTrue(tc, avg == 30000);
}

void AvgCalc_noAnswerOnce(CuTest* tc) {
    unsigned short values[] = {100, 200, USHRT_MAX, 400};

    unsigned short avg = AvgCalc(values, sizeof(values) / sizeof(unsigned short));

    CuAssertTrue(tc, avg == 233);
}

void AvgCalc_noAnswer(CuTest* tc) {
    unsigned short values[] = {USHRT_MAX, USHRT_MAX, USHRT_MAX};

    unsigned short avg = AvgCalc(values, sizeof(values) / sizeof(unsigned short));

    CuAssertTrue(tc, avg == USHRT_MAX);
}

void FastRound_test1(CuTest* tc) {
    unsigned short rounded = FastRound(3.295);

    CuAssertTrue(tc, rounded == 3);
}

void FastRound_test2(CuTest* tc) {
    unsigned short rounded = FastRound(3.5);

    CuAssertTrue(tc, rounded == 4);
}

void FastRound_test3(CuTest* tc) {
    unsigned short rounded = FastRound(3.7);

    CuAssertTrue(tc, rounded == 4);
}

void FastRound_test4(CuTest* tc) {
    unsigned short rounded = FastRound(3);

    CuAssertTrue(tc, rounded == 3);
}

int main(int argc, char *argv[]) {
    printf("HTTP ping utility tests\n\r");

    CuSuite* testSuite = CuSuiteNew();
    CuString* result = CuStringNew();

    SUITE_ADD_TEST(testSuite, AvgCalc_smallValues);
    SUITE_ADD_TEST(testSuite, AvgCalc_overflow);
    SUITE_ADD_TEST(testSuite, AvgCalc_noAnswerOnce);
    SUITE_ADD_TEST(testSuite, AvgCalc_noAnswer);

    SUITE_ADD_TEST(testSuite, FastRound_test1);
    SUITE_ADD_TEST(testSuite, FastRound_test2);
    SUITE_ADD_TEST(testSuite, FastRound_test3);
    SUITE_ADD_TEST(testSuite, FastRound_test4);

    CuSuiteRun(testSuite);
    CuSuiteSummary(testSuite, result);
    CuSuiteDetails(testSuite, result);

    printf("%s\n\r", result->buffer);
}