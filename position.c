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
#include "dobutsu.h"

/*
 * Check if a and b refer to the same position, return nonzero if and
 * only if they do.  We need this function instead of just memcmp()
 * since the same position can be represented in multiple ways.
 */
extern int
position_equal(const struct position *a, const struct position *b)
{
	size_t i;
	static const unsigned char prom_flip[4] = { 0, 2, 1, 3 };

	if (gote_moves(a) != gote_moves(b))
		return (0);

	/* fast path, should be enough for most positions */
	if (a->map != b->map)
		return (0);

	if ((a->pieces[CHCK_S] != b->pieces[CHCK_S]
	   || a->pieces[CHCK_G] != b->pieces[CHCK_G]
	   || a->status != b->status)
	   && (a->pieces[CHCK_S] != b->pieces[CHCK_G]
	   || a->pieces[CHCK_G] != b->pieces[CHCK_S]
	   || (a->status & 3) != prom_flip[b->status & 3]))
		return (0);

	for (i = 2; i < PIECE_COUNT; i += 2)
		if ((a->pieces[i] != b->pieces[i] || a->pieces[i + 1] != b->pieces[i + 1])
		    && (a->pieces[i] != b->pieces[i + 1] || a->pieces[i + 1] != b->pieces[i]))
			return (0);

	return (1);
}
