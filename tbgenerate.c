#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "dobutsutable.h"

static void	initial_round(struct tablebase *);
static void	initial_round_pos(struct tablebase *, poscode, unsigned *, unsigned *);
static int	normal_round(struct tablebase *, int);
static void	normal_round_pos(struct tablebase *, poscode, int, unsigned *, unsigned *);

/*
 * This function generates a complete tablebase and returns the
 * generated table base or NULL in case of error with errno containing
 * the reason for failure.  Progress information may be printed to
 * stderr in the process.
 */
extern struct tablebase *
generate_tablebase(void)
{
	struct tablebase *tb = calloc(sizeof *tb, 1);
	int round;

	if (tb == NULL)
		return (NULL);

	initial_round(tb);
	round = 2;
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
	int game_ended;

	decode_poscode(&p, pc);
	if (gote_in_check(&p)) {
		tb->positions[offset] = 1;
		++*win1;
		return;
	}

	nmove = generate_moves(moves, &p);
	for (i = 0; i < nmove; i++) {
		struct position pp = p;
		game_ended = play_move(&pp, moves[i]);
		assert(!game_ended);

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
		size_t j, naliases;
		poscode aliases[MAX_PCALIAS];

		undo_move(&pp, unmoves[i]);

		/*
		 * optimization: save encode_position() call for
		 * positions that are also mate in 1.
		 */
		if (sente_in_check(&pp))
			continue;

		naliases = poscode_aliases(aliases, &pp);
		for (j = 0; j < naliases; j++) {
			offset = position_offset(aliases[j]);
			if (tb->positions[offset] == 0)
				tb->positions[offset] = 2;
		}
	}
}

/*
 * In all but the first round we examine all positions that were marked
 * as wins for this round, do a retrograde analysis on them and then for
 * each position we find this way, we check if it's a losing position.
 * If it is, we mark the position as "lost" with the appropriate
 * distance to mate and every position reachable unmarked positions from
 * this as "won" with the appropriate distance to mate. If any positions
 * were marked in this last phase, nonzero is returned, zero otherwise.
 */
static int
normal_round(struct tablebase *tb, int round)
{
	poscode pc;
	unsigned size, wins = 0, losses = 0;

	fprintf(stderr, "Round %3d: ", round);

	for (pc.cohort = 0; pc.cohort < COHORT_COUNT; pc.cohort++) {
		size = cohort_size[pc.cohort].size;
		for (pc.ownership = 0; pc.ownership < OWNERSHIP_COUNT; pc.ownership++)
			for (pc.lionpos = 0; pc.lionpos < LIONPOS_COUNT; pc.lionpos++)
				for (pc.map = 0; pc.map < size; pc.map++)
					normal_round_pos(tb, pc, round, &wins, &losses);
	}

	fprintf(stderr, "%9u  %9u\n", wins, losses);

	return (losses != 0);
}

/*
 * Process one position in a normal round.
 */
static void
normal_round_pos(struct tablebase *tb, poscode pc, int round,
    unsigned *wins, unsigned *losses)
{
	struct position p;
	struct unmove unmoves[MAX_UNMOVES];
	size_t i, nunmove;

	if (tb->positions[position_offset(pc)] != round)
		return;

	++*wins;

	decode_poscode(&p, pc);
	nunmove = generate_unmoves(unmoves, &p);
	for (i = 0; i < nunmove; i++) {
		/* check if this is indeed a losing position */
		struct position pp = p;
		struct unmove ununmoves[MAX_UNMOVES];
		struct move moves[MAX_MOVES];
		poscode aliases[MAX_PCALIAS];
		tb_entry value;
		size_t j, k, nalias, nununmove, nmove;

		undo_move(&pp, unmoves[i]);

		/* have we already analyzed this position? */
		if (lookup_position(tb, &pp) != 0)
			continue;

		/* make sure all moves are losing */
		nmove = generate_moves(moves, &pp);
		for (j = 0; j < nmove; j++) {
			struct position ppp = pp;

			play_move(&ppp, moves[j]);
			value = lookup_position(tb, &ppp);
			if (!is_win(value) || value > round)
				goto not_a_losing_position;
		}

		/* all moves are losing, mark positions as lost */
		nalias = poscode_aliases(aliases, &pp);
		for (j = 0; j < nalias; j++)
			if (tb->positions[position_offset(aliases[j])] == 0) {
				tb->positions[position_offset(aliases[j])] = -round;
				++*losses;
			}

		/* mark all positions reachable from this one as won */
		nununmove = generate_unmoves(ununmoves, &pp);
		for (j = 0; j < nununmove; j++) {
			struct position ppp = pp;

			undo_move(&ppp, ununmoves[j]);

			nalias = poscode_aliases(aliases, &ppp);
			for (k = 0; k < nalias; k++)
				if (lookup_poscode(tb, aliases[k]) == 0)
					tb->positions[position_offset(aliases[k])] = round + 1;
		}

	not_a_losing_position:
		;
	}
}
