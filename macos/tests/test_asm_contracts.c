#undef NDEBUG
#include "fix.h"
#include "vecmat.h"

#include <assert.h>

int oflow_check(fix a, fix b);

/* VECMAT_C retains the original debug hooks. */
void Assert(int expression) { assert(expression); }
void Int3(void) { assert(!"unexpected Int3"); }

int main(void)
{
    vms_vector x;
    vms_vector y;
    vms_vector cross;

    assert(oflow_check(F1_0, F1_0) == 0);
    assert(oflow_check(i2f(30000), i2f(2)) != 0);

    assert(vm_vec_make(&x, F1_0, 0, 0) == &x);
    assert(vm_vec_make(&y, 0, F1_0, 0) == &y);
    assert(vm_vec_dot(&x, &x) == F1_0);
    assert(vm_vec_dot(&x, &y) == 0);
    assert(vm_vec_cross(&cross, &x, &y) == &cross);
    assert(cross.x == 0);
    assert(cross.y == 0);
    assert(cross.z == F1_0);
    return 0;
}
