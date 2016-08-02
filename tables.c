#include "dobutsu.h"

/*
 * all tables have a 13th entry to indicate "in hand"
 */

/* sente chick */
const board Cmoves[13] = {
	00010, 00020, 00040,
	00100, 00200, 00400,
	01000, 02000, 04000,
	00000, 00000, 00000,
	00000
};

/* gote chick */
const board cmoves[13] = {
	00000, 00000, 00000,
	00001, 00002, 00004,
	00010, 00020, 00040,
	00100, 00200, 00400,
	00000
};

/* sente rooster */
const board Rmoves[13] = {
	00032, 00075, 00062,
	00321, 00752, 00624,
	03210, 07520, 06240,
	02100, 05200, 02400,
	00000
};

/* gote rooster */
const board rmoves[13] = {
	00012, 00025, 00042,
	00123, 00257, 00426,
	01230, 02570, 04260,
	02300, 05700, 02600,
	00000
};

/* lion */
const board Llmoves[13] = {
	00032, 00075, 00062,
	00323, 00757, 00626,
	03230, 07570, 06260,
	02300, 05700, 02600,
	00000
};

/* giraffe */
const board Ggmoves[13] = {
	00012, 00025, 00042,
	00121, 00252, 00424,
	01210, 02520, 04240,
	02100, 05200, 02400,
	00000
};

/* elephants */
const board Eemoves[13] = {
	00020, 00050, 00020,
	00202, 00505, 00202,
	02020, 05050, 02020,
	00200, 00500, 00200,
	00000
};

/*
 * lookup table for mirroring the board vertically and turning it
 * by 180 degrees.  We don't do a horizontal mirror as that would
 * violate parity more often than a 180 degree flip.
 */
const unsigned char vert_mirror[13] = {
	0x2, 0x1, 0x0,
	0x5, 0x4, 0x3,
	0x8, 0x7, 0x6,
	0xb, 0xa, 0x9,
	0xc
};

const unsigned char turn_board[13] = {
	0xb, 0xa, 0x9,
	0x8, 0x7, 0x6,
	0x5, 0x4, 0x3,
	0x2, 0x1, 0x0,
	0xc
};
