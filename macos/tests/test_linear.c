#undef NDEBUG
#include "LINEAR_C.H"

#include <assert.h>
#include <string.h>

static unsigned char pixels[8 * 8];

static void reset_pixels(void)
{
    memset(pixels, 0, sizeof(pixels));
    gr_var_bitmap = pixels;
    gr_var_bwidth = 8;
    gr_var_color = 7;
}

static int count_color(void)
{
    int count = 0;
    unsigned index;
    for (index = 0; index < sizeof(pixels); ++index)
        if (pixels[index] == 7)
            ++count;
    return count;
}

int main(void)
{
    unsigned char forward[sizeof(pixels)];

    reset_pixels();
    gr_linear_line(2, 1, 2, 5);
    assert(count_color() == 5);
    assert(pixels[1 * 8 + 2] == 7);
    assert(pixels[5 * 8 + 2] == 7);

    reset_pixels();
    gr_linear_line(1, 3, 6, 3);
    assert(count_color() == 6);

    reset_pixels();
    gr_linear_line(1, 1, 6, 6);
    assert(count_color() == 6);
    memcpy(forward, pixels, sizeof(pixels));
    reset_pixels();
    gr_linear_line(6, 6, 1, 1);
    assert(memcmp(forward, pixels, sizeof(pixels)) == 0);

    reset_pixels();
    gr_linear_line(6, 1, 1, 4);
    memcpy(forward, pixels, sizeof(pixels));
    reset_pixels();
    gr_linear_line(1, 4, 6, 1);
    assert(memcmp(forward, pixels, sizeof(pixels)) == 0);
    return 0;
}
