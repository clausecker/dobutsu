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
 * to order them.
 */
static int
compare_analysis(const void *ap, const void *bp)
{
	const struct analysis *a = ap, *b = bp;

	return (wdl_compare(a->value, b->value));
}

/*
 * Generate an analysis for p, store it to an and return the number of
 * moves found.  Sort the result by position value such that the best
 * move is first (i.e. the move leading to the worst position for the
 * opponent).
 */
extern size_t
analyze_position(struct analysis an[MAX_MOVES],
    const struct tablebase *tb, const struct position *p)
{
	struct move moves[MAX_MOVES];
	size_t i, nmove;

	nmove = generate_moves(moves, p);
	for (i = 0; i < nmove; i++) {
		an[i].pos = *p;
		an[i].move = moves[i];
		play_move(&an[i].pos, moves[i]);
		an[i].value = lookup_position(tb, &an[i].pos);
	}

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
	double values[MAX_MOVES], total = 0.0, rngval;

	size_t i, nmove;

	assert(strength >= 0);

	nmove = analyze_position(an, tb, p);

	/*
	 * While there are two positions where no moves are available,
	 * I assume that these aren't reachable.  This assumption might
	 * be wrong, so TODO: check this.
	 */
	assert(nmove > 0);

	/* on max level, play perfectly */
	if (strength >= MAX_STRENGTH)
		return (an[0].move);

	/*
	 * Mal each position rating to a positive floating point value.
	 * we use the mapping function exp(strength / value) which
	 * hopefully does the trick.
	 */
	for (i = 0; i < nmove; i++)
		values[i] = is_draw(an[i].value) ? 1.0 : exp(strength / an[i].value);

	/* replace values with accumulate values */
	for (i = 0; i < nmove; i++)
		values[i] = total += values[i];

	rngval = erand48(s->xsubi) * total;
	for (i = 0; i < nmove; i++) {
		if (rngval < values[i])
			return an[i].move;
	}

	/* UNREACHABLE */
	assert(0);
}
