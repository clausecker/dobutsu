#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "dobutsutable.h"

static void	initial_round(struct tablebase *);
static void	initial_round_pos(struct tablebase *, poscode, unsigned *, unsigned *);
static int	normal_round(struct tablebase *, int);
static void	normal_round_pos(struct tablebase *, poscode, int, unsigned *, unsigned *);
static unsigned	mark_position(struct tablebase *, const struct position *, tb_entry);
static void	count_wdl(const struct tablebase *);

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

	count_wdl(tb);

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

	fprintf(stderr, "Round  1: ");

	for (pc.cohort = 0; pc.cohort < COHORT_COUNT; pc.cohort++) {
		size = cohort_size[pc.cohort].size;
		for (pc.lionpos = 0; pc.lionpos < LIONPOS_COUNT; pc.lionpos++)
			for (pc.map = 0; pc.map < size; pc.map++)
				for (pc.ownership = 0; pc.ownership < OWNERSHIP_COUNT; pc.ownership++)
					if (has_valid_ownership(pc))
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

		undo_move(&pp, unmoves[i]);

		/*
		 * optimization: save encode_position() call for
		 * positions that are also mate in 1.
		 */
		if (!sente_in_check(&pp))
			mark_position(tb, &pp, 2);
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

	fprintf(stderr, "Round %2d: ", round);

	for (pc.cohort = 0; pc.cohort < COHORT_COUNT; pc.cohort++) {
		size = cohort_size[pc.cohort].size;
		for (pc.lionpos = 0; pc.lionpos < LIONPOS_COUNT; pc.lionpos++)
			for (pc.map = 0; pc.map < size; pc.map++)
				for (pc.ownership = 0; pc.ownership < OWNERSHIP_COUNT; pc.ownership++)
					if (has_valid_ownership(pc))
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
		struct position ppmirror, pp = p;
		poscode pc;
		struct unmove ununmoves[MAX_UNMOVES];
		struct move moves[MAX_MOVES];
		tb_entry value;
		size_t j, nununmove, nmove, offset;

		undo_move(&pp, unmoves[i]);

		/* have we already analyzed this position? */
		encode_position(&pc, &pp);
		if (pc.lionpos >= LIONPOS_COUNT)
			continue;

		offset = position_offset(pc);
		if (tb->positions[offset] != 0)
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
		assert(tb->positions[offset] == 0 || tb->positions[offset] == -round);
		if (tb->positions[offset] == 0)
			++*losses;

		tb->positions[offset] = -round;

		ppmirror = pp;
		if (position_mirror(&ppmirror)) {
			encode_position(&pc, &ppmirror);
			offset = position_offset(pc);
			assert(tb->positions[offset] == 0 || tb->positions[offset] == -round);
			if (tb->positions[offset] == 0)
				++*losses;

			tb->positions[offset] = -round;
		}

		/* mark all positions reachable from this one as won */
		nununmove = generate_unmoves(ununmoves, &pp);
		for (j = 0; j < nununmove; j++) {
			struct position ppp = pp;

			undo_move(&ppp, ununmoves[j]);

			if (!gote_in_check(&ppp))
				mark_position(tb, &ppp, round + 1);
		}

	not_a_losing_position:
		;
	}
}

/*
 * Mark position p and its mirrored variant as e in tb if it hasn't been
 * marked before. Return the number of newly marked positions.
 */
static unsigned
mark_position(struct tablebase *tb, const struct position *p, tb_entry e)
{
	struct position pp = *p;
	poscode pc;
	size_t offset;
	unsigned count = 0;

	encode_position(&pc, &pp);
	offset = position_offset(pc);
	if (tb->positions[offset] == 0) {
		count++;
		tb->positions[offset] = e;
	}

	if (!position_mirror(&pp))
		return (count);

	encode_position(&pc, &pp);
	offset = position_offset(pc);
	if (tb->positions[offset] == 0) {
		count++;
		tb->positions[offset] = e;
	}

	return (count);
}

/*
 * Count how many positions are wins, draws, and losses and print the
 * figures to stderr.
 */
static void
count_wdl(const struct tablebase *tb)
{
	poscode pc;
	unsigned size, win = 0, draw = 0, loss = 0;
	tb_entry e;

	for (pc.cohort = 0; pc.cohort < COHORT_COUNT; pc.cohort++) {
		size = cohort_size[pc.cohort].size;
		for (pc.lionpos = 0; pc.lionpos < LIONPOS_COUNT; pc.lionpos++)
			for (pc.map = 0; pc.map < size; pc.map++)
				for (pc.ownership = 0; pc.ownership < OWNERSHIP_COUNT; pc.ownership++)
					if (has_valid_ownership(pc)) {
						e = tb->positions[position_offset(pc)];
						if (is_win(e))
							win++;
						else if (is_loss(e))
							loss++;
						else /* is_draw(e) */
							draw++;
					}
	}

	fprintf(stderr, "Total:    %9u  %9u  %9u\n", win, loss, draw);
}

/*
 * Write tb to file f.  It is assumed that f has been opened in binary
 * mode for writing and truncated.  This function returns 0 on success,
 * -1 on error with errno indicating the reason for failure.
 */
extern int
write_tablebase(FILE *f, const struct tablebase *tb)
{

	fwrite(tb->positions, sizeof tb->positions, 1, f);
	fflush(f);

	return (ferror(f) ? -1 : 0);
}
