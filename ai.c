#define _POSIX_C_SOURCE 200809L
#include <stdlib.h>
#include <time.h>

#include "tablebase.h"

static int	compare_analysis(const void*, const void*);
static size_t	unsorted_analysis(struct analysis[MAX_MOVES],
    const struct tablebase*, const struct position*);

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
 * Generate an analysis for p, store it to an and return the number of
 * moves found.  Do not sort the result.
 */
static size_t
unsorted_analysis(struct analysis an[MAX_MOVES],
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

	return (nmove);
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
 * move is first (i.e. that leading to the worst position for the
 * opponent).
 */
extern size_t
analyze_position(struct analysis an[MAX_MOVES],
    const struct tablebase *tb, const struct position *p)
{
	size_t nmove;

	nmove = unsorted_analysis(an, tb, p);
	qsort(an, nmove, sizeof *an, compare_analysis);

	return (nmove);
}
