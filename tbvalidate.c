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

	for (pc.ownership = 0; pc.ownership < OWNERSHIP_TOTAL_COUNT; pc.ownership++)
		for (pc.cohort = 0; pc.cohort < COHORT_COUNT; pc.cohort++) {
			if (!has_valid_ownership(pc))
				continue;

			size = cohort_size[pc.cohort].size;
			for (pc.lionpos = 0; pc.lionpos < LIONPOS_COUNT; pc.lionpos++)
				for (pc.map = 0; pc.map < size; pc.map++)
					result &= validate_position(tb, pc);
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
	struct position p, bestp;
	struct move moves[MAX_MOVES], bestmove;
	size_t i, nmove;
	tb_entry bestvalue = 1, actual;

	decode_poscode(&p, pc);
	actual = lookup_position(tb, &p);

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

	/* compare all moves and see which one is the best */
	nmove = generate_moves(moves, &p);
	for (i = 0; i < nmove; i++) {
		struct position pp = p;
		int game_ended;
		tb_entry e;

		game_ended = play_move(&pp, moves + i);
		assert (game_ended == 0);

		e = lookup_position(tb, &pp);

		/* the best move leads to the worst position (for the opponent) */
		if (wdl_compare(bestvalue, e) >= 0) {
			bestvalue = e;
			bestmove = moves[i];
			bestp = pp;
		}
	}

	if (next_dtm(actual) != bestvalue) {
		char posstr[MAX_POSSTR], movstr[MAX_MOVSTR];

		position_string(posstr, &p);
		move_string(movstr, &p, &bestmove);
		fprintf(stderr, "%-24s (%3d) => %-7s => ", posstr, (int)actual, movstr);
		position_string(posstr, &bestp);
		fprintf(stderr, "%-24s (%3d) should be %3d\n",
		    posstr, (int)bestvalue, (int)next_dtm(actual));

		return (0);
	}

	return (1);
}
