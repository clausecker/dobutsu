#include "dobutsu.h"

/*
 * This table is used by moves_for() to lookup the moves for all pieces.
 * The first dimension is the piece number shifted right by one, the
 * second dimension is the square the piece is on.  Moves for roosters
 * are encoded in a separate table.
 */
#define MOVETAB_ENTRY(X0, X1, X2, X3, X4, X5, X6, X7, X8, X9, X10, X11) \
	{ X0, X1, X2, X3, X4, X5, X6, X7, \
	X8, X9, X10, X11, BOARD_S, 0, 0, 0, \
	X0 << GOTE_PIECE, X1 << GOTE_PIECE, X2 << GOTE_PIECE, X3 << GOTE_PIECE, \
	X4 << GOTE_PIECE, X5 << GOTE_PIECE, X6 << GOTE_PIECE, X7 << GOTE_PIECE, \
	X8 << GOTE_PIECE, X9 << GOTE_PIECE, X10 << GOTE_PIECE, X11 << GOTE_PIECE, \
	BOARD_G, 0, 0, 0 }

const board movetab[PIECE_COUNT/2][32] = {
	[CHCK_S/2] = {
	    00010, 00020, 00040, 00100, 00200, 00400, 01000, 02000,
	    04000, 00000, 00000, 00000, BOARD_S, 0, 0, 0,
	    00000 << GOTE_PIECE, 00000 << GOTE_PIECE, 00000 << GOTE_PIECE, 00001 << GOTE_PIECE,
	    00002 << GOTE_PIECE, 00004 << GOTE_PIECE, 00010 << GOTE_PIECE, 00020 << GOTE_PIECE,
	    00040 << GOTE_PIECE, 00100 << GOTE_PIECE, 00200 << GOTE_PIECE, 00400 << GOTE_PIECE,
	    BOARD_G, 0, 0, 0
	},
	[GIRA_S/2] = MOVETAB_ENTRY(00012, 00025, 00042, 00121, 00252, 00424,
	    01210, 02520, 04240, 02100, 05200, 02400),
	[ELPH_S/2] = MOVETAB_ENTRY(00020, 00050, 00020, 00202, 00505, 00202,
	    02020, 05050, 02020, 00200, 00500, 00200),
	[LION_S/2] = MOVETAB_ENTRY(00032, 00075, 00062, 00323, 00757, 00626,
	    03230, 07570, 06260, 02300, 05700, 02600)
};

/*
 * Separate move 
 */
const board roostertab[32] = {
	00032, 00075, 00062, 00321, 00752, 00624, 03210, 07520,
	06240, 02100, 05200, 02400, BOARD_S, 0, 0, 0,
	00012 << GOTE_PIECE, 00025 << GOTE_PIECE, 00042 << GOTE_PIECE, 00123 << GOTE_PIECE,
	00257 << GOTE_PIECE, 00426 << GOTE_PIECE, 01230 << GOTE_PIECE, 02570 << GOTE_PIECE,
	04260 << GOTE_PIECE, 02300 << GOTE_PIECE, 05700 << GOTE_PIECE, 02600 << GOTE_PIECE,
	BOARD_G, 0, 0, 0
};

/*
 * Compute the possible moves for piece pc in p.  Both moving into
 * check and not moving out of check comprises a legal move.
 */
extern board
moves_for(unsigned pc, const struct position *p)
{
	board dst;

	if (is_promoted(pc, p))
		dst = roostertab[p->pieces[pc]];
	else
		dst = movetab[pc >> 1][p->pieces[pc]];

	/* remove invalid destination squares */
	dst &= ~p->map;

	if (piece_in(HAND, p->pieces[pc]))
		dst &= ~swap_colors(p->map);

	return dst;
}
