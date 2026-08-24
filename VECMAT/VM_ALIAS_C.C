/*
 * MACOS C replacements for the Watcom alias/inline assembly entry points in
 * VECMAT.H.  The preserved assembly declares vm_vec_dot and vm_vec_cross as
 * alternate public names for vm_vec_dotprod and vm_vec_crossprod, and writes
 * x/y/z directly in vm_vec_make.  These wrappers preserve that ABI contract
 * while delegating all arithmetic to the portable implementations in
 * VECMAT_C.C.
 */
#include "vecmat.h"

vms_vector *vm_vec_make(vms_vector *v, fix x, fix y, fix z)
{
	v->x = x;
	v->y = y;
	v->z = z;
	return v;
}

fix vm_vec_dot(vms_vector *v0, vms_vector *v1)
{
	return vm_vec_dotprod(v0, v1);
}

vms_vector *vm_vec_cross(vms_vector *dest, vms_vector *src0, vms_vector *src1)
{
	return vm_vec_crossprod(dest, src0, src1);
}
