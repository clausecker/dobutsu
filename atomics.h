/*-
 * Copyright (c) 2016--2017 Robert Clausecker. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
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

/* clang uses this */
#ifndef __has_builtin
# define __has_builtin(x) 0
#endif

/* C11 atomics */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__STDC_NO_ATOMICS_)
# include <stdatomic.h>
_Static_assert(sizeof (atomic_schar) == 1,
    "This program does not work on systems where an atomic_schar is larger than 1 byte");
#elif __has_builtin(__sync_lock_test_and_set) \
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
