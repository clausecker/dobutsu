/*
 * This is the non-generic interface to the position and move
 * structures.  It contains some convenience functions to effectively
 * analyse and modify these structures.
 */

#include <stddef.h>
#include <stdint.h>

#include "rules.h"

/* a bitmap for squares on the board */
typedef uint_fast32_t board;

/* various bitmaps to build boards */
enum {
        BOARD_S = 007777,
	PROMZ_S = 007000,
	PROMZ_G = 000007 << GOTE_PIECE,
        BOARD_G = BOARD_S << GOTE_PIECE,
        BOARD   = BOARD_S | BOARD_G,
        HAND_S  = 010000,
        HAND_G  = HAND_S << GOTE_PIECE,
        HAND    = HAND_S | HAND_G,
        VALID_SQUARE = BOARD | HAND,
        MAX_SQUARE = GOTE_PIECE + SQUARE_COUNT,
};

static inline	int	square_valid(unsigned);
static inline	int	piece_in(board, unsigned);
static inline	int	piece_in_nosg(board, unsigned);
static inline	int	gote_owns(unsigned);
static inline	board	swap_colors(board);
static inline	board	board_map(const struct position*);
extern		board	attack_map(const struct position*);
extern		board	moves_for(size_t, const struct position*);

/* inline implementations */

/*
 * Return 1 if sq is a values value for a
 * struct position.pieces[] member.
 */
static inline int
square_valid(unsigned sq)
{

	return !!((sq & ~GOTE_PIECE) <= PIECE_COUNT);
}

/*
 * Return 1 if the bitmap indicates 1 for sq.
 */
static inline int
piece_in(board b, unsigned sq)
{

	return !!(b & 1 << sq);
}

/*
 * Return 1 if the bitmap indicates 1 for sq, not
 * accounting for the difference between sente and gote.
 */
static inline int
piece_in_nosg(board b, unsigned sq)
{
	return !!((b | b >> GOTE_PIECE) & 1 << (sq & ~GOTE_PIECE));
}

/*
 * Return 1 if gote owns piece pc.
 */
static inline int
gote_owns(unsigned pc)
{

	return !!(pc & GOTE_PIECE);
}

/*
 * Compute a bitmap of all pieces on the board for pos.
 * Pieces in hand are ignored.
 */
static inline board
board_map(const struct position *p)
{
	size_t i;
	board b = 0;

	for (i = 0; i < PIECE_COUNT; i++)
		b |= 1 << p->pieces[i];

	return (b & BOARD);
}

/*
 * swap the colours in a board.
 */
static inline
board swap_colors(board b)
{
	return (b << GOTE_PIECE | b >> GOTE_PIECE);
}
