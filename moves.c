#include <assert.h>

#include "dobutsu.h"

/*
 * Returns 1 if Sente is in check, that is, if Gote could take the
 * Sente lion if it was Gote's move.  In addition, this function also
 * returns 1 if Gote could ascend if it was Gote's move.  If neither
 * applies, 0 is returned.
 */
extern int
sente_in_check(const struct position *p)
{
	board b = attack_map(p);

	/* in check? */
	if (piece_in(b, p->pieces[LION_S]))
		return (1);

	/* can ascend? */
	if (moves_for(LION_G, p) & PROMZ_G)
		return (1);

	return (0);
}

/*
 * Returns 1 if Gote is in check, that is, if Sente could take the
 * Gote lion if it was Sente's move.  In addition, this function also
 * returns 1 if Sente could ascend if it was Sente's move.  If neither
 * applies, 0 is returned.
 */
extern int
gote_in_check(const struct position *p)
{
	board b = attack_map(p);

	/* in check? */
	if (piece_in(b, p->pieces[LION_G]))
		return (1);

	/* can ascend? */
	if (moves_for(LION_S, p) & PROMZ_S)
		return (1);

	return (0);
}

/*
 * Turn the board 180 degrees such that Sente becomes Gote and vice
 * versa.  Also flip who plays the next move.
 */
extern void
turn_board(struct position *p)
{
	size_t i;
	unsigned tmp;

	static unsigned char turned_board[] = {
		[ 0] = GOTE_PIECE | 11,
		[ 1] = GOTE_PIECE | 10,
		[ 2] = GOTE_PIECE |  9,
		[ 3] = GOTE_PIECE |  8,
		[ 4] = GOTE_PIECE |  7,
		[ 5] = GOTE_PIECE |  6,
		[ 6] = GOTE_PIECE |  5,
		[ 7] = GOTE_PIECE |  4,
		[ 8] = GOTE_PIECE |  3,
		[ 9] = GOTE_PIECE |  2,
		[10] = GOTE_PIECE |  1,
		[11] = GOTE_PIECE |  0,
		[IN_HAND] = GOTE_PIECE | IN_HAND,

		[GOTE_PIECE |  0] = 11,
		[GOTE_PIECE |  1] = 10,
		[GOTE_PIECE |  2] =  9,
		[GOTE_PIECE |  3] =  8,
		[GOTE_PIECE |  4] =  7,
		[GOTE_PIECE |  5] =  6,
		[GOTE_PIECE |  6] =  5,
		[GOTE_PIECE |  7] =  4,
		[GOTE_PIECE |  8] =  3,
		[GOTE_PIECE |  9] =  2,
		[GOTE_PIECE | 10] =  1,
		[GOTE_PIECE | 11] =  0,
		[GOTE_PIECE | IN_HAND] = IN_HAND,
	};

	p->map = 0;
	for (i = 0; i < PIECE_COUNT; i++) {
		p->pieces[i] = turned_board[p->pieces[i]];
		p->map |= 1 << p->pieces[i];
	}

	/* exchange lions */
	tmp = p->pieces[LION_S];
	p->pieces[LION_S] = p->pieces[LION_G];
	p->pieces[LION_G] = tmp;

	p->map &= BOARD;
	null_move(p);	
}

/*
 * Generate all moves for pc and add them to moves.
 */
static struct move *
generate_moves_for_piece(struct move *moves, const struct position *p, size_t pc)
{
	size_t i, i0 = gote_moves(p) * GOTE_PIECE;
	board dest_squares = moves_for(pc, p);

	for (i = i0; i < i0 + SQUARE_COUNT; i++)
		if (piece_in(dest_squares, i))
			*moves++ = (struct move){ .piece = pc, .to = i };

	return (moves);
}

/*
 * Generate all moves for the player who has the right to move. Return
 * the number of moves generated.  If both pieces of one kind are in
 * hand, only attempt to drop one of them.
 */
extern size_t
generate_moves(struct move moves[MAX_MOVES], const struct position *p)
{
	struct move *om = moves;
	size_t mc = 0, i;

	for (i = 0; i < PIECE_COUNT; i += 2) {
		if (gote_moves(p) == gote_owns(p->pieces[i]))
			moves = generate_moves_for_piece(moves, p, i);

		/* only drop one piece of each kind if both are in hand */
		if (p->pieces[i] != p->pieces[i + 1] &&
		    gote_moves(p) == gote_owns(p->pieces[i + 1]))
			moves = generate_moves_for_piece(moves, p, i + 1);
	}

	mc = (size_t)(moves - om);
	assert (mc <= MAX_MOVES);
	return (mc);
}

/*
 * Generate unmove structures for piece pc that might have captured any
 * the pieces listed in uncap of length ucc while moving.
 */
static struct unmove *
generate_unmoves_for_piece(struct unmove *unmoves, const struct position *p,
    size_t pc, const size_t *uncap, size_t ucc)
{
	struct unmove um;
	int gote_moved = !gote_moves(p);
	size_t i, j, i0 = gote_moved * GOTE_PIECE;
	board src_squares = unmoves_for(pc, p);

	um.piece = pc;

	for (i = i0; i < i0 + SQUARE_COUNT; i++) {
		if (!piece_in(src_squares, i))
			continue;

		um.from = i;
		um.status = 0;
		um.capture = -1;
		*unmoves++ = um;

		/* account for capture */
		for (j = 0; j < ucc; j++) {
			um.status = 0;
			um.capture = uncap[j];
			*unmoves++ = um;

			/* account for rooster capture */
			if (uncap[j] == CHCK_S || uncap[j] == CHCK_G) {
				um.status = 1 << uncap[j];
				*unmoves++ = um;
			}
		}
	}

	/* account for drop */
	if (pc != LION_S && pc != LION_G && !is_promoted(pc, p)) {
		um.from = i0 + IN_HAND;
		um.status = 0,
		um.capture = -1;
		*unmoves++ = um;
	}

	/* account for chicken promoting to rooster */
	if (is_promoted(pc, p)
	    && piece_in(gote_moved ? PROMZ_G : PROMZ_S, p->pieces[pc])
	    && !piece_in_nosg(p->map, p->pieces[pc] + (gote_moved ? 3 : -3))) {
		um.from = p->pieces[pc] + (gote_moved ? 3 : -3);
		um.status = 1 << pc;
		um.capture = -1;
		*unmoves++ = um;

		/* account for capture */
		for (j = 0; j < ucc; j++) {
			um.status = 1 << pc;
			um.capture = uncap[j];
			*unmoves++ = um;

			/* account for rooster capture */
			if (uncap[j] == CHCK_S || uncap[j] == CHCK_G) {
				um.status |= 1 << uncap[j];
				*unmoves++ = um;
			}
		}
	}

	return (unmoves);
}

/*
 * Generate unmove structures for all moves would lead to this position.
 */
extern size_t
generate_unmoves(struct unmove unmoves[MAX_UNMOVES], const struct position *p)
{
	struct unmove *oum = unmoves;
	size_t umc, i, uncap[PIECE_COUNT], ucc = 0;
	int gote_moved = !gote_moves(p);

	/*
	 * Check which pieces we can uncapture.  If both pieces of a
	 * kind could be uncaptured, only uncapture one of them.
	 */
	for (i = 0; i < PIECE_COUNT; i += 2) {
		if (p->pieces[i] == gote_moved * GOTE_PIECE + IN_HAND)
			uncap[ucc++] = i;
		else if (p->pieces[i + 1] == gote_moved * GOTE_PIECE + IN_HAND)
			uncap[ucc++] = i + 1;
	}

	for (i = 0; i < PIECE_COUNT; i++)
		if (gote_moved == gote_owns(p->pieces[i]) && piece_in(BOARD, p->pieces[i]))
			unmoves = generate_unmoves_for_piece(unmoves, p, i, uncap, ucc);

	umc = (size_t)(unmoves - oum);
	assert (umc <= MAX_UNMOVES);
	return (umc);
}

/*
 * Play move m on position p.  No sanity checks are performed.  This
 * function returns 1 if the move played ended the game by taking the
 * opponent's lion or by ascending.
 */
extern int
play_move(struct position *p, struct move m)
{
	unsigned status = p->status;
	int ret = 0, i;

	/* update occupation map to board state after move */
	p->map &= ~(1 << p->pieces[m.piece]);
	p->map &= ~(1 << (m.to ^ GOTE_PIECE));
	p->map |= 1 << m.to;
	p->map &= BOARD;

	/* do promotion and ascension */
	if (!piece_in(HAND, p->pieces[m.piece])
	    && piece_in(gote_moves(p) ? PROMZ_G : PROMZ_S, m.to)) {
		status |= 1 << m.piece;

		/* did an ascension happen? Check if lion is in danger. */
		if (status & (1 << LION_S | 1 << LION_G))
			ret = !piece_in(attack_map(p), m.to);

	}

	p->pieces[m.piece] = m.to;

	/* clear promotion bits for pieces that can't be promoted */
	status &= POS_FLAGS;

	/* do capture */
	for (i = 0; i < PIECE_COUNT; i++)
		if (p->pieces[i] == (m.to ^ GOTE_PIECE))
			break;

	if (i < PIECE_COUNT) {
		/* move captured piece to hand and flip ownership */
		p->pieces[i] = p->pieces[i] & GOTE_PIECE ^ (IN_HAND | GOTE_PIECE);

		/* unpromote captured piece */
		status &= ~(1 << i);

		/* check for captured king */
		if (i == LION_S || i == LION_G)
			ret = 1;
	}

	p->status = status ^ GOTE_MOVES;

	return (ret);
}

/*
 * Undo a move described by a struct umove.
 */
extern void
undo_move(struct position *p, struct unmove u)
{
	p->map &= ~(1 << p->pieces[u.piece]);
	p->map |= 1 << u.from;

	if (u.capture >= 0) {
		p->pieces[u.capture] = p->pieces[u.piece] ^ GOTE_PIECE;
		p->map |= 1 << p->pieces[u.capture];
	}

	p->map &= BOARD;
	p->pieces[u.piece] = u.from;

	p->status ^= u.status | GOTE_MOVES;
}
