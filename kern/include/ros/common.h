#ifndef ROS_COMMON_H
#define ROS_COMMON_H

#ifndef __ASSEMBLER__

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>
#include <stdbool.h>

typedef uintptr_t physaddr_t;
typedef long intreg_t;
typedef unsigned long uintreg_t;

#ifndef NULL
#define NULL ((void*) 0)
#endif

#ifndef TRUE
#define TRUE	1
#endif

#ifndef FALSE
#define FALSE	0
#endif

#define FOR_CIRC_BUFFER(next, size, var) \
	for (int _var = 0, var = (next); _var < (size); _var++, var = (var + 1) % (size))

#define STRINGIFY(s) __STRINGIFY(s)
#define __STRINGIFY(s) #s

/* A macro for testing if another macro has been #defined or not.  Can be used
 * wherever you need a boolean defined.  Returns 0 or 1. */
#define is_defined(macro) is_defined_(macro)
#define is_defined_test_1 ,
#define is_defined_(value) is_defined__(is_defined_test_##value, value)
#define is_defined__(comma, value) is_defined___(comma 1, 0)
#define is_defined___(_, v, ...) v

// Efficient min and max operations
#ifdef ROS_KERNEL /* Glibc or other user libs have their own */
#define MIN(_a, _b)						\
({								\
	typeof(_a) __a = (_a);					\
	typeof(_b) __b = (_b);					\
	__a <= __b ? __a : __b;					\
})
#define MAX(_a, _b)						\
({								\
	typeof(_a) __a = (_a);					\
	typeof(_b) __b = (_b);					\
	__a >= __b ? __a : __b;					\
})

/* Other kernel-only includes */

/* Test for alignment, e.g. 2^6 */
#define ALIGNED(p, a)	(!(((uintptr_t)(p)) & ((a)-1)))
/* Aligns x up to the mask, e.g. (2^6 - 1) (round up if any mask bits are set)*/
#define __ALIGN_MASK(x, mask) (((uintptr_t)(x) + (mask)) & ~(mask))
/* Aligns x up to the alignment, e.g. 2^6. */
#define ALIGN(x, a) ((typeof(x)) __ALIGN_MASK(x, (a) - 1))
/* Will return false for 0.  Debatable, based on what you want. */
#define IS_PWR2(x) ((x) && !((x) & (x - 1)))

#define ARRAY_SIZE(x) (sizeof((x))/sizeof((x)[0]))

#endif

/* Rounding operations (efficient when n is a power of 2)
 * Round down to the nearest multiple of n.
 * The compiler should compile out the branch.  This is needed for 32 bit, so
 * that we can round down uint64_t, without chopping off the top 32 bits. */
#define ROUNDDOWN(a, n)                                                        \
({                                                                             \
	typeof(a) __b;                                                             \
	if (sizeof(a) == 8) {                                                      \
		uint64_t __a = (uint64_t) (a);                                         \
		__b = (typeof(a)) (__a - __a % (n));                                   \
	} else {                                                                   \
		uintptr_t __a = (uintptr_t) (a);                                       \
		__b = (typeof(a)) (__a - __a % (n));                                   \
	}                                                                          \
	__b;                                                                       \
})

/* Round up to the nearest multiple of n */
#define ROUNDUP(a, n)                                                          \
({                                                                             \
	typeof(a) __b;                                                             \
	if (sizeof(a) == 8) {                                                      \
		uint64_t __n = (uint64_t) (n);                                         \
		__b = (typeof(a)) (ROUNDDOWN((uint64_t) (a) + __n - 1, __n));          \
	} else {                                                                   \
		uintptr_t __n = (uintptr_t) (n);                                       \
		__b = (typeof(a)) (ROUNDDOWN((uintptr_t) (a) + __n - 1, __n));         \
	}                                                                          \
	__b;                                                                       \
})

#define DIV_ROUND_UP(n,d) (((n) + (d) - 1) / (d))

// Return the integer logarithm of the value provided rounded down
static inline uintptr_t LOG2_DOWN(uintptr_t value)
{
	value |= 1;  // clz(0) is undefined, just or in a 1 bit and define it
	// intrinsic __builtin_clz supported by both > gcc4.6 and LLVM > 1.5
	return (sizeof(value) == 8) ? 63 - __builtin_clzll(value)
	                            : 31 - __builtin_clz(value);
}

// Return the integer logarithm of the value provided rounded up
static inline uintptr_t LOG2_UP(uintptr_t value)
{
	uintptr_t ret = LOG2_DOWN(value);
	ret += 0 != (value ^ ((uintptr_t) 1 << ret));  // Add 1 if a lower bit set
	return ret;
}

static inline uintptr_t ROUNDUPPWR2(uintptr_t value)
{
	return 1 << LOG2_UP(value);
}

static inline uintptr_t ROUNDDOWNPWR2(uintptr_t value)
{
	return 1 << LOG2_DOWN(value);
}

/* We wraparound if UINT_MAX < a * b, which is also UINT_MAX / a < b. */
static inline bool mult_will_overflow_u64(uint64_t a, uint64_t b)
{
	if (!a)
		return FALSE;
	return (uint64_t)(-1) / a < b;
}

// Return the offset of 'member' relative to the beginning of a struct type
#ifndef offsetof
#define offsetof(type, member)  ((size_t) (&((type*)0)->member))
#endif

/* Return the container/struct holding the object 'ptr' points to */
#define container_of(ptr, type, member) ({                                     \
	(type*)((char*)ptr - offsetof(type, member));                             \
})

/* Force the reading exactly once of x.  You may still need mbs().  See
 * http://lwn.net/Articles/508991/ for more info. */
#define ACCESS_ONCE(x) (*(volatile typeof(x) *)&(x))

/* Makes sure func is run exactly once.  Can handle concurrent callers, and
 * other callers spin til the func is complete. */
#define run_once(func)                                                         \
{                                                                              \
	static bool ran_once = FALSE;                                              \
	static atomic_t is_running = FALSE;                                        \
	if (!ran_once) {                                                           \
		if (!atomic_swap(&is_running, TRUE)) {                                 \
			/* we won the race and get to run the func */                      \
			func;                                                              \
			wmb();	/* don't let the ran_once write pass previous writes */    \
			ran_once = TRUE;                                                   \
		} else {                                                               \
			/* someone else won, wait til they are done to break out */        \
			while (!ran_once)                                                  \
				cpu_relax();                                                   \
                                                                               \
		}                                                                      \
	}                                                                          \
}

/* Unprotected, single-threaded version, makes sure func is run exactly once */
#define run_once_racy(func)                                                    \
{                                                                              \
	static bool ran_once = FALSE;                                              \
	if (!ran_once) {                                                           \
		func;                                                                  \
		ran_once = TRUE;                                                       \
	}                                                                          \
}

/* Aborts with 'retcmd' if this function has already been called.  Compared to
 * run_once, this is put at the top of a function that can be called from
 * multiple sources but should only execute once. */
#define init_once_racy(retcmd)                                                 \
{                                                                              \
	static bool initialized = FALSE;                                           \
	if (initialized) {                                                         \
		retcmd;                                                                \
	}                                                                          \
	initialized = TRUE;                                                        \
}

#endif /* __ASSEMBLER__ */

#endif /* ROS_COMMON_H */
