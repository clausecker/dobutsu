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
#define _XOPEN_SOURCE 700L /* for erand() */
#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <time.h>

#include "tablebase.h"

static int	compare_analysis(const void*, const void*);

/*
 * Generate a random seed for use with ai_move().  Currently, the
 * current system time is used but that is implementation defined.
 */
extern void
ai_seed(struct seed *s)
{
	struct timespec tp;

	clock_gettime(CLOCK_REALTIME, &tp);
	s->xsubi[0] = tp.tv_nsec & 0xffffU;
	s->xsubi[1] = tp.tv_nsec >> 16 & 0xffffU;
	s->xsubi[2] = tp.tv_sec & 0xffffU;
}

/*
 * Compare two struct analysis by their value member.  Use wdl_compare
 * to order them such that the best move is first.
 */
static int
compare_analysis(const void *ap, const void *bp)
{
	const struct analysis *a = ap, *b = bp;

	return (-wdl_compare(a->entry, b->entry));
}

/*
 * Generate an analysis for p, store it to an and return the number of
 * moves found.  Sort the result by position value such that the best
 * move is first (i.e. the move leading to the worst position for the
 * opponent).  The strength member indicates the engine strength used
 * for computing the value member.
 */
extern size_t
analyze_position(struct analysis an[MAX_MOVES],
    const struct tablebase *tb, const struct position *p, double strength)
{
	struct position pp;
	struct move moves[MAX_MOVES];
	double total = 0.0;
	size_t i, nmove;

	nmove = generate_moves(moves, p);
	for (i = 0; i < nmove; i++) {
		pp = *p;
		an[i].move = moves[i];
		if (play_move(&pp, moves + i))
			an[i].entry = 1;
		else
			an[i].entry = prev_dtm(lookup_position(tb, &pp));

		total += an[i].value = an[i].entry == 0.0 ?
		    1.0 : exp(strength / an[i].entry);
	}

	assert(nmove == 0 || total > 0);

	/* normalize value members */
	for (i = 0; i < nmove; i++)
		an[i].value /= total;

	qsort(an, nmove, sizeof *an, compare_analysis);

	return (nmove);
}

/*
 * Using the seed s, generate an ai move for position p, looking up
 * position evaluations from tb.  strength indicates the AI strength,
 * an integer between 0 (play random moves) and MAX_STRENGTH.  If
 * strength is equal to or larger than MAX_STRENGTH, the AI plays
 * perfectly.
 */
extern struct move
ai_move(const struct tablebase *tb, const struct position *p,
    struct seed *s, double strength)
{
	struct analysis an[MAX_MOVES];
	double rngval;

	size_t i, nmove;

	assert(strength >= 0);

	nmove = analyze_position(an, tb, p, strength);

	/*
	 * While there are two positions where no moves are available,
	 * I assume that these aren't reachable.  This assumption might
	 * be wrong, so TODO: check this.
	 */
	assert(nmove > 0);

	/* on max level, play perfectly */
	if (strength >= MAX_STRENGTH) {
		/* randomly select one move from all best moves */
		i = 1;

		for (i = 1; i < nmove && an[0].entry == an[i].entry; i++)
			;

		i *= erand48(s->xsubi);

		assert(0 <= i && i < nmove);

		return (an[i].move);
	}

	/* select random move according to evaluation */
	rngval = erand48(s->xsubi);
	for (i = 0; i < nmove; i++) {
		if (rngval < an[i].value)
			return an[i].move;
		else
			rngval -= an[i].value;
	}

	/* due to rounding errors, it might happen that no value matches */
	return (an[0].move);
}
