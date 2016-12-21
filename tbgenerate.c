#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "dobutsutable.h"

static void	initial_round(struct tablebase *);
static void	initial_round_pos(struct tablebase *, poscode, unsigned *, unsigned *);
static int	normal_round(struct tablebase *, int);

/*
 * This function generates a complete tablebase and returns the
 * generated table base or NULL in case of error with errno containing
 * the reason for failure.  Progress information may be printed to
 * stderr in the process.
 */
extern struct tablebase *
generate_tablebase(void)
{
	struct tablebase *tb = malloc(sizeof *tb);
	int round;

	if (tb == NULL)
		return (NULL);

	initial_round(tb);
	round = 1;
	while (normal_round(tb, round))
		round++;

	return (tb);
}

/*
 * In the initial round, every positions in the tablebase is evaluated.
 * Positions are categorized as:
 *
 *  - immediate wins (1) if gote_in_check() holds
 *  - checkmates (-1) if for each possible move sente_in_check() holds.
 *    This includes stalemates.
 *  - mate-in-one positions (2) if a checkmate can be reached.
 */
static void
initial_round(struct tablebase *tb)
{
	poscode pc;
	unsigned size, win1 = 0, loss1 = 0;

	fprintf(stderr, "Round   1: ");

	for (pc.cohort = 0; pc.cohort < COHORT_COUNT; pc.cohort++) {
		size = cohort_size[pc.cohort].size;
		for (pc.ownership = 0; pc.ownership < OWNERSHIP_COUNT; pc.ownership++)
			for (pc.lionpos = 0; pc.lionpos < LIONPOS_COUNT; pc.lionpos++)
				for (pc.map = 0; pc.map < size; pc.map++)
					initial_round_pos(tb, pc, &win1, &loss1);
	}

	fprintf(stderr, "%9u  %9u\n", win1, loss1);
}

/*
 * For the initial round, evaluate one position indicated by pc and
 * store the result in tb.  Also increment win1 and loss1 if an
 * immediate win or checkmate is encountered.
 */
static void
initial_round_pos(struct tablebase *tb, poscode pc, unsigned *win1, unsigned *loss1)
{
	struct position p;
	struct unmove unmoves[MAX_UNMOVES];
	struct move moves[MAX_MOVES];
	size_t i, nmove, offset = position_offset(pc);

	decode_poscode(&p, pc);
	if (gote_in_check(&p)) {
		tb->positions[offset] = 1;
		++*win1;
		return;
	}

	nmove = generate_moves(moves, &p);
	for (i = 0; i < nmove; i++) {
		struct position pp = p;
		play_move(&pp, moves[i]);

		if (!sente_in_check(&pp)) {
			/* position is not an immediate loss, can't judge it */
			return;
		}
	}

	/* all moves lead to a win for Gote */
	tb->positions[offset] = -1;
	++*loss1;
	nmove = generate_unmoves(unmoves, &p);
	for (i = 0; i < nmove; i++) {
		struct position pp = p;
		poscode pcpc;

		undo_move(&pp, unmoves[i]);
		encode_position(&pcpc, &pp);
		if (pcpc.lionpos >= LIONPOS_COUNT)
			continue;

		offset = position_offset(pcpc);
		if (tb->positions[offset] == 0)
			tb->positions[offset] = 2;
	}
}

// TODO
static int normal_round(struct tablebase *tb, int round) { return 0; }
