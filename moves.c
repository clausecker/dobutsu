#include <assert.h>

#include "dobutsu.h"

/*
 * This table is used by moves_for() to lookup the moves for all pieces.
 * The first dimension is the piece number shifted right by one, the
 * second dimension is the square the piece is on.  Moves for roosters
 * are encoded in a separate table.  No bits are set for pieces in hand.
 */
#define MOVETAB_ENTRY(X0, X1, X2, X3, X4, X5, X6, X7, X8, X9, X10, X11) \
	{ X0, X1, X2, X3, X4, X5, X6, X7, \
	X8, X9, X10, X11, 0, 0, 0, 0, \
	X0 << GOTE_PIECE, X1 << GOTE_PIECE, X2 << GOTE_PIECE, X3 << GOTE_PIECE, \
	X4 << GOTE_PIECE, X5 << GOTE_PIECE, X6 << GOTE_PIECE, X7 << GOTE_PIECE, \
	X8 << GOTE_PIECE, X9 << GOTE_PIECE, X10 << GOTE_PIECE, X11 << GOTE_PIECE, \
	0, 0, 0, 0 }

static const board movetab[PIECE_COUNT/2][32] = {
	[CHCK_S/2] = {
	    00010, 00020, 00040, 00100, 00200, 00400, 01000, 02000,
	    04000, 00000, 00000, 00000, 0, 0, 0, 0,
	    00000 << GOTE_PIECE, 00000 << GOTE_PIECE, 00000 << GOTE_PIECE, 00001 << GOTE_PIECE,
	    00002 << GOTE_PIECE, 00004 << GOTE_PIECE, 00010 << GOTE_PIECE, 00020 << GOTE_PIECE,
	    00040 << GOTE_PIECE, 00100 << GOTE_PIECE, 00200 << GOTE_PIECE, 00400 << GOTE_PIECE,
	    0, 0, 0, 0
	},
	[GIRA_S/2] = MOVETAB_ENTRY(00012, 00025, 00042, 00121, 00252, 00424,
	    01210, 02520, 04240, 02100, 05200, 02400),
	[ELPH_S/2] = MOVETAB_ENTRY(00020, 00050, 00020, 00202, 00505, 00202,
	    02020, 05050, 02020, 00200, 00500, 00200),
	[LION_S/2] = MOVETAB_ENTRY(00032, 00075, 00062, 00323, 00757, 00626,
	    03230, 07570, 06260, 02300, 05700, 02600)
};

/*
 * A chick promotes to a rooster.  These are the moves for such a piece.
 * This table is used by unmoves.c, too.
 */
const board roostertab[32] = {
	00032, 00075, 00062, 00321, 00752, 00624, 03210, 07520,
	06240, 02100, 05200, 02400, 0, 0, 0, 0,
	00012 << GOTE_PIECE, 00025 << GOTE_PIECE, 00042 << GOTE_PIECE, 00123 << GOTE_PIECE,
	00257 << GOTE_PIECE, 00426 << GOTE_PIECE, 01230 << GOTE_PIECE, 02570 << GOTE_PIECE,
	04260 << GOTE_PIECE, 02300 << GOTE_PIECE, 05700 << GOTE_PIECE, 02600 << GOTE_PIECE,
	0, 0, 0, 0
};

/*
 * Compute a bitmap of all attacked squares.  Colours are swapped such
 * that fields attacked by Gote are marked for Sente and vice versa.
 */
static board
attack_map(const struct position *p)
{
	board b;

	b  = is_promoted(CHCK_S, p) ? roostertab[p->pieces[CHCK_S]] : movetab[CHCK_S / 2][p->pieces[CHCK_S]];
	b |= is_promoted(CHCK_G, p) ? roostertab[p->pieces[CHCK_G]] : movetab[CHCK_G / 2][p->pieces[CHCK_G]];
	b |= movetab[GIRA_S / 2][p->pieces[GIRA_S]];
	b |= movetab[GIRA_G / 2][p->pieces[GIRA_G]];
	b |= movetab[ELPH_S / 2][p->pieces[ELPH_S]];
	b |= movetab[ELPH_G / 2][p->pieces[ELPH_G]];
	b |= movetab[LION_S / 2][p->pieces[LION_S]];
	b |= movetab[LION_G / 2][p->pieces[LION_G]];

	return (swap_colors(b));
}

/*
 * Compute the possible moves for piece pc in p.  Both moving into
 * check and not moving out of check comprises a legal move.
 */
extern board
moves_for(unsigned pc, const struct position *p)
{
	board dst;

	if (piece_in(HAND, p->pieces[pc])) {
		/* was: dst = gote_owns(p->pieces[pc]) ? BOARD_G : BOARD_S; */
		dst = BOARD_S << p->pieces[pc] - IN_HAND;
		dst &= ~swap_colors(p->map);
	} else
		dst = is_promoted(pc, p) ? roostertab[p->pieces[pc]] : movetab[pc / 2][p->pieces[pc]];

	/* don't allow lion to move to attacked squares */
	if (pc == LION_S || pc == LION_G)
		dst &= ~attack_map(p);

	/* remove invalid destination squares */
	dst &= ~p->map;

	return dst;
}

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
