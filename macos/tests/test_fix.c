#undef NDEBUG
#include "fix.h"

#include <assert.h>
#include <stdint.h>

static int near_value(int value, int expected, int tolerance)
{
    int difference = value - expected;
    return difference >= -tolerance && difference <= tolerance;
}

int main(void)
{
    fix sine = 123;
    fix cosine = 123;

    assert(sizeof(fix) == 4);
    assert(sizeof(fixang) == 2);
    assert(fixmul(F1_0, F1_0) == F1_0);
    assert(fixmul(fl2f(1.5), i2f(2)) == i2f(3));
    assert(fixmul(-F0_5, F0_5) == -0x4000);
    assert(fixdiv(i2f(3), i2f(2)) == fl2f(1.5));
    assert(fixdiv(-F1_0, i2f(2)) == -F0_5);
    assert(fixmuldiv(i2f(6), i2f(3), i2f(2)) == i2f(9));

    fix_sincos(0, &sine, &cosine);
    assert(sine == 0);
    assert(cosine == F1_0);
    fix_sincos(0x4000, &sine, &cosine);
    assert(near_value(sine, F1_0, 1));
    assert(near_value(cosine, 0, 1));
    fix_fastsincos(0x407f, &sine, NULL);
    assert(near_value(sine, F1_0, 1));

    assert(fix_asin(F1_0) == 0x4000);
    assert(fix_acos(0) == 0x4000);
    assert(fix_atan2(F1_0, 0) == 0);
    assert(fix_atan2(0, F1_0) == 0x4000);

    assert(long_sqrt(-1) == 0);
    assert(long_sqrt(0) == 0);
    assert(long_sqrt(2) == 1);
    /* Preserve Parallax's integer Newton iteration, which returns 1 here. */
    assert(long_sqrt(3) == 1);
    assert(long_sqrt(65536) == 256);
    assert(quad_sqrt(0, 1) == 65536);
    assert(quad_sqrt(0, -1) == 0);
    assert(fix_sqrt(F1_0) == F1_0);
    assert(near_value(fix_sqrt(i2f(4)), i2f(2), 1));
    return 0;
}
