#include <assert.h>
#include <stdio.h>

#include "dobutsutable.h"

static int validate_position(const struct tablebase *, poscode);

/*
 * Check if the tablebase tb is internally consistent.  If any
 * error is found, information is printed to stderr.  This function
 * returns 1 on success, 0 on failure.
 */
extern int
validate_tablebase(const struct tablebase *tb)
{
	poscode pc;
	unsigned size;
	int result = 1;

	for (pc.cohort = 0; pc.cohort < COHORT_COUNT; pc.cohort++) {
		size = cohort_size[pc.cohort].size;
		for (pc.ownership = 0; pc.ownership < OWNERSHIP_COUNT; pc.ownership++) {
			if (!has_valid_ownership(pc))
				continue;

			for (pc.lionpos = 0; pc.lionpos < LIONPOS_COUNT; pc.lionpos++)
				for (pc.map = 0; pc.map < size; pc.map++)
					result &= validate_position(tb, pc);
		}
	}

	return (result);
}

/*
 * Validate a single position by checking every position reachable from it and
 * making sure, that it's one better than the best reachable result.
 */
static int
validate_position(const struct tablebase *tb, poscode pc)
{
	struct position p, pstrongest;
	struct move moves[MAX_MOVES], bestmove;
	size_t i, nmove;
	int game_ended;
	tb_entry expected, bestvalue, actual;

	actual = tb->positions[position_offset(pc)];
	decode_poscode(&p, pc);

	/*
	 * since we never look them up, don't care about positions
	 * marked as checkmate.
	 */
	if (gote_in_check(&p))
		return (1);

	nmove = generate_moves(moves, &p);
	if (nmove == 0) {
		char posstr[MAX_POSSTR];

		if (actual == -1)
			return (1);

		position_string(posstr, &p);
		fprintf(stderr, "%-24s (%3d) => (none)  => should be -1\n", posstr, (int)actual);
		return (0);
	}

	/* initialize expectations with first move */
	pstrongest = p;
	bestmove = moves[0];
	game_ended = play_move(&pstrongest, moves[0]);
	if (game_ended) {
		expected = 1;
		bestvalue = 0;
	} else {
		bestvalue = lookup_position(tb, &pstrongest);
		if (bestvalue > 0)
			expected = -bestvalue;
		else if (bestvalue < 0)
			expected = -bestvalue + 1;
		else
			expected = 0;
	}			

	for (i = 1; i < nmove; i++) {
		struct position pp = p;
		tb_entry value;
		int game_ended;

		game_ended = play_move(&pp, moves[i]);

		/* can't trump immediate win */
		if (game_ended) {
			pstrongest = pp;
			expected = 1;
			bestvalue = -1;
			bestmove = moves[i];
			break;
		}

		value = lookup_position(tb, &pp);
		if (is_loss(value)) {
			if (!is_win(expected) || get_dtm(value) + 1 < get_dtm(expected)) {
				pstrongest = pp;
				bestvalue = value;
				expected = 1 - value;
				bestmove = moves[i];
			}
		} else if (is_draw(value)) {
			if (is_loss(expected)) {
				pstrongest = pp;
				bestvalue = value;
				expected = 0;
				bestmove = moves[i];
			}
		} else /* is_win(value) */ {
			if (is_loss(expected) && 1 + get_dtm(value) > get_dtm(expected)) {
				pstrongest = pp;
				bestvalue = value;
				expected = -value;
				bestmove = moves[i];
			}
		}
	}

	if (actual != expected) {
		char posstr[MAX_POSSTR], movstr[MAX_MOVSTR];

		position_string(posstr, &p);
		move_string(movstr, &p, bestmove);
		fprintf(stderr, "%-24s (%3d) => %-7s => ", posstr, (int)actual, movstr);
		position_string(posstr, &pstrongest);
		fprintf(stderr, "%-24s (%3d) should be %3d\n", posstr, (int)bestvalue, (int)expected);

		return (0);
	}

	return (1);
}
