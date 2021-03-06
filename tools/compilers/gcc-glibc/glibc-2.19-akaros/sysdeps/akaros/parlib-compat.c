/* Copyright (c) 2015 Google Inc.
 * Kevin Klues <klueska@cs.berkeley.edu>
 * See LICENSE for details.
 */

#include <libc-symbols.h>
#include <parlib/stdio.h>
#include <parlib/assert.h>

/* Here we define functions that are really defined in parlib, but we need
 * them in libc in order to link it. We weak alias them here so that the
 * parlib definitions will override them later. Unfortunately, this trick
 * only works so long as we leave parlib as a static library. If we ever
 * decide to make parlib a .so, then we will have to revisit this and use
 * function pointers at runtime or something similar. */

int __akaros_printf(const char *format, ...)
{
	assert(0);
	return -1;
}
weak_alias(__akaros_printf, akaros_printf)

void ___assert_failed(const char *file, int line, const char *msg)
{
	assert(0);
}
weak_alias(___assert_failed, _assert_failed)
