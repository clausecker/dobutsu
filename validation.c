#include "dobutsu.h"

/*
 * Check if p describes a valid position.  The following invariants are
 * checked:
 *
 *  - check all fields for invalid values
 *  - no two pieces may occupy the same square
 *  - LION_S must be owned by Sente, LION_G by Gote
 *  - LION_S and LION_G must be on the board
 *  - LION_S cannot occupy the fourth rank, LION_G not the first
 *  - a chick in hand cannot be promoted
 *  - the board map must correspond to the pieces
 */
extern int
position_valid(const struct position *p)
{
	size_t i;
	board b;

	for (i = 0; i < PIECE_COUNT; i++)
		if (!square_valid(p->pieces[i]))
			return (0);

	for (i = b = 0; i < PIECE_COUNT; i++) {
		if (b & 1 << p->pieces[i] || b & 1 << (p->pieces[i] ^ GOTE_PIECE))
			return (0);

		/* ignore pieces in hand in overlap check */
		b |= 1 << p->pieces[i] & ~HAND;
	}

	if (b != p->map) {
		return (0);
	}

	if (!piece_in(BOARD_S & ~PROMZ_S, p->pieces[LION_S]) || !piece_in(BOARD_G & ~ PROMZ_G, p->pieces[LION_G]))
		return (0);

	if (piece_in(HAND, p->pieces[CHCK_S]) && p->status & ROST_S
	    || piece_in(HAND, p->pieces[CHCK_G]) && p->status & ROST_G)
		return (0);

	return (1);
}

/*
 * Check if m describes a valid move, assuming that p describes a valid
 * position.  The following invariants are checked:
 *
 *  - the piece number must be between 0 and PIECE_COUNT - 1
 *  - the player to play must own the piece in question
 *  - the destination square must have a valid number
 *  - the GOTE_PIECE bit of the destination square must agree to
      the bit indicating who moves
 *  - if the piece is dropped, the destination square must be empty,
 *    else the destination square must be reachable and not occupied by
 *    our own pieces
 */
extern int
move_valid(const struct position *p, struct move m)
{

	if (m.piece >= PIECE_COUNT)
		return (0);

	if (gote_moves(p) ^ gote_owns(p->pieces[m.piece]))
		return (0);

	if (gote_moves(p) ^ gote_owns(m.to))
		return (0);

	if (!square_valid(m.to) || piece_in(HAND, m.to))
		return (0);

	if (!piece_in(moves_for(m.piece, p), m.to))
		return (0);

	if (piece_in(HAND, p->pieces[m.piece])) {
		if (piece_in_nosg(p->map, m.to))
			return (0);
	} else {
		if (piece_in(p->map, m.to))
			return (0);
	}

	return (1);
}
