#include <stdio.h>
#include <math.h>

#include "logmath.h"

#define TEST_ASSERT(x) if (!(x)) { fprintf(stderr, "FAIL: %s\n", #x); exit(1); }
#define EPSILON 0.001
#define TEST_EQUAL(a,b) TEST_ASSERT((a) == (b))
#define TEST_EQUAL_FLOAT(a,b) TEST_ASSERT(fabs((a) - (b)) < EPSILON)
#define TEST_EQUAL_LOG(a,b) TEST_ASSERT(abs((a) - (b)) < -logmath_log(lmath,EPSILON))
