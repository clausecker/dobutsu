#ifndef ATOMICS_H
#define ATOMICS_H

/*
 * The gentb program requires an atomic exchange primitive to accurately
 * keep track of how many positions it evaluated.  This header either
 * supplies C11 or gcc primitives, depending on what is available.
 *
 * These macros define the following macros and types:
 *  atomic_schar -- an atomic signed char type
 *  atomic_exchange() -- a C11 like atomic exchange macro
 */

/* C11 atomics */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS_)
# include <stdatomic.h>
_Static_assert(sizeof (atomic_schar) == 1,
    "This program does not work on systems where an atomic_schar is larger than 1 byte");
#elif defined(__has_builtin) && __has_builtin(__sync_lock_test_and_set) \
    || defined(__GNUC__) && __GNUC__ >= 4
/* gcc __sync functions */
typedef volatile signed char atomic_schar;
# define atomic_exchange __sync_lock_test_and_set
#else
/* no atomic primitives */
#define NO_ATOMICS
typedef signed char atomic_schar;


static inline
atomic_schar atomic_exchange(atomic_schar *x, atomic_schar c)
{
	atomic_schar old = *x;

	*x = c;
	return (old);
}
#endif

#endif /* ATOMICS_H */
