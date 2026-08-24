/*
 * MACOS C replacement for the Watcom x86 `oflow_check` block in FVI.C.
 *
 * Assembly contract:
 *   - inputs: signed 16.16 fixed-point values a and b
 *   - computes abs(a) * abs(b) as a full-width product
 *   - returns non-zero when the product cannot be represented as a signed
 *     16.16 fixed-point value after shifting right by 16
 *
 * Keeping this implementation adjacent to FVI.C makes the replacement easy
 * to compare independently with the preserved assembly block.
 */
#include <stdint.h>
#include "fix.h"

int oflow_check(fix a, fix b)
{
	uint64_t magnitude_a = a < 0 ? (uint64_t)(-(int64_t)a) : (uint64_t)a;
	uint64_t magnitude_b = b < 0 ? (uint64_t)(-(int64_t)b) : (uint64_t)b;
	return magnitude_a * magnitude_b > ((uint64_t)INT32_MAX << 16);
}
