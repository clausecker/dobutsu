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
	size_t i;
	board b = 0;

	for (i = 0; i < PIECE_COUNT; i++)
		if (piece_in(BOARD, p->pieces[i]))
			b |= moves_for(i, p);

	b = swap_colors(b);

	/* in check? */
	if (piece_in(b, p->pieces[LION_S]))
		return (1);

	/* can ascend? */
	if (~b & moves_for(LION_G, p) & PROMZ_G)
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
	size_t i;
	board b = 0;

	for (i = 0; i < PIECE_COUNT; i++)
		if (piece_in(BOARD, p->pieces[i]))
			b |= moves_for(i, p);

	b = swap_colors(b);

	/* in check? */
	if (piece_in(b, p->pieces[LION_G]))
		return (1);

	/* can ascend? */
	if (~b & moves_for(LION_S, p) & PROMZ_S)
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
	size_t i;

	for (i = 0; i < PIECE_COUNT; i++)
		p->pieces[i] = turned_board[p->pieces[i]];

	null_move(p);	
}

/*
 * Generate all moves for pc and add them to moves.
 */
static size_t
generate_moves_for_piece(struct move *moves, const struct position *p, size_t pc)
{
	size_t mc = 0, i, n = SQUARE_COUNT + gote_moves(p) * GOTE_PIECE;
	board dest_squares = moves_for(pc, p);

	for (i = gote_moves(p) * GOTE_PIECE; i < n; i++)
		if (dest_squares & (1 << i))
			moves[mc++] = (struct move){ .piece = pc, .to = i };

	return (mc);
}

/*
 * Generate all moves for the player who has the right to move. Return
 * the number of moves generated.
 */
extern size_t
generate_moves(struct move moves[MAX_MOVES], const struct position *p)
{
	size_t mc = 0, i;
	unsigned piece_mask = gote_moves(p) ? GOTE_PIECE : 0;

	for (i = 0; i < PIECE_COUNT; i++)
		if (gote_moves(p) == gote_owns(p->pieces[i]))
			mc += generate_moves_for_piece(moves + mc, p, i);

	return (mc);
}
