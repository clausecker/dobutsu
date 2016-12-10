#include <assert.h>

#include "dobutsutable.h"

static void	normalize_position(struct position *);
static unsigned	encode_ownership(const struct position *);
static void	encode_pieces(poscode *, struct position *);
static void	place_pieces(struct position *, unsigned, unsigned);
static void	assign_ownership(struct position *, unsigned);

/*
 * Encode a position structure into a tablebase index (poscode).  On
 * success, 0 is returned, in case of failure, -1.  It is assumed that
 * p encodes a valid position.
 */
extern int
encode_position(poscode *pc, const struct position *pos)
{
	struct position p = *pos;

	normalize_position(&p);
	pc->ownership = encode_ownership(&p);
	encode_pieces(pc, &p);

	return (0);
}


/*
 * Decode a tablebase index (poscode) into a position structure.  On
 * success, 0 is returned, in case of failure, -1.  It is assumed that
 * pc is the output of encode_position() for a valid position.
 */
extern int
decode_poscode(struct position *pos, const poscode *pc)
{

	place_pieces(pos, pc->cohort, pc->map);
	assign_ownership(pos, pc->ownership);

	return (0);
}

/*
 * Normalize p, that means:
 *  - if it's Gote to move, turn the board 180 degrees.
 *  - if the Sente lion is on the right board-half, flip the board
 *    along the center file
 */
static void
normalize_position(struct position *p)
{
	size_t i;
	static const unsigned char flipped_board[] = {
		[ 0] = 2,
		[ 1] = 1,
		[ 2] = 0,
		[ 3] = 5,
		[ 4] = 4,
		[ 5] = 3,
		[ 6] = 8,
		[ 7] = 7,
		[ 8] = 6,
		[ 9] = 11,
		[10] = 10,
		[11] = 9,
		[IN_HAND] = IN_HAND,

		[GOTE_PIECE | 0] = GOTE_PIECE | 2,
		[GOTE_PIECE | 1] = GOTE_PIECE | 1,
		[GOTE_PIECE | 2] = GOTE_PIECE | 0,
		[GOTE_PIECE | 3] = GOTE_PIECE | 5,
		[GOTE_PIECE | 4] = GOTE_PIECE | 4,
		[GOTE_PIECE | 5] = GOTE_PIECE | 3,
		[GOTE_PIECE | 6] = GOTE_PIECE | 8,
		[GOTE_PIECE | 7] = GOTE_PIECE | 7,
		[GOTE_PIECE | 8] = GOTE_PIECE | 6,
		[GOTE_PIECE | 9] = GOTE_PIECE | 11,
		[GOTE_PIECE |10] = GOTE_PIECE | 10,
		[GOTE_PIECE |11] = GOTE_PIECE | 9,
		[GOTE_PIECE |IN_HAND] = GOTE_PIECE | IN_HAND,
	};

	if (gote_moves(p))
		turn_board(p);

	if (piece_in(00444, p->pieces[LION_S]))
		for (i = 0; i < PIECE_COUNT; i++)
			p->pieces[i] = flipped_board[p->pieces[i]];
}

/*
 * Encode who owns what piece into a bitmap between 0 and 64.
 */
static unsigned
encode_ownership(const struct position *p)
{
	unsigned result = 0;

	if (gote_owns(p->pieces[CHCK_S]))
		result |= 1 << 0;
	if (gote_owns(p->pieces[CHCK_G]))
		result |= 1 << 1;
	if (gote_owns(p->pieces[GIRA_S]))
		result |= 1 << 2;
	if (gote_owns(p->pieces[GIRA_G]))
		result |= 1 << 3;
	if (gote_owns(p->pieces[ELPH_S]))
		result |= 1 << 4;
	if (gote_owns(p->pieces[ELPH_G]))
		result |= 1 << 5;

	return (result);
}

/*
 * Decode who owns what piece and assign the result to the pieces in p.
 */
static void
assign_ownership(struct position *p, unsigned os)
{

	if (os & 1 << 0)
		p->pieces[CHCK_S] |= GOTE_PIECE;
	if (os & 1 << 1)
		p->pieces[CHCK_G] |= GOTE_PIECE;
	if (os & 1 << 2)
		p->pieces[GIRA_S] |= GOTE_PIECE;
	if (os & 1 << 3)
		p->pieces[GIRA_G] |= GOTE_PIECE;
	if (os & 1 << 4)
		p->pieces[ELPH_S] |= GOTE_PIECE;
	if (os & 1 << 5)
		p->pieces[ELPH_G] |= GOTE_PIECE;
}

/*
 * After encoding the ownership information, the two pieces of each
 * kind may be swapped for encoding.  This table contains swapped
 * encoding bits for each piece pair such that one can, after preparing
 * a suitable mask, simply write
 *
 *     ownership ^= mask & owner_flip[ownership];
 *
 * to flip the onwership of those piece pairs mentioned in mask. The
 * table prom_flip contains a map with the promotion bits for Sente
 * and Gote chicks flipped.
 */
static const unsigned char owner_flip[64] = {
	0x00, 0x03, 0x03, 0x00, 0x0c, 0x0f, 0x0f, 0x0c,
	0x0c, 0x0f, 0x0f, 0x0c, 0x00, 0x03, 0x03, 0x00,
	0x30, 0x33, 0x33, 0x30, 0x3c, 0x3f, 0x3f, 0x3c,
	0x3c, 0x3f, 0x3f, 0x3c, 0x30, 0x33, 0x33, 0x30,
	0x30, 0x33, 0x33, 0x30, 0x3c, 0x3f, 0x3f, 0x3c,
	0x3c, 0x3f, 0x3f, 0x3c, 0x30, 0x33, 0x33, 0x30,
	0x00, 0x03, 0x03, 0x00, 0x0c, 0x0f, 0x0f, 0x0c,
	0x0c, 0x0f, 0x0f, 0x0c, 0x00, 0x03, 0x03, 0x00,
}, prom_flip[4] = { 0, 2, 1, 3 };

/*
 * A map from bits indicating which pieces are on the board to cohort
 * numbers. To cut down the number of cohorts, it is assumed that if
 * the _G piece is on the board, then the _S piece is on the board, too.
 * Entries not corresponding to any cohort are marked -1 (0xff).
 */
static const unsigned char cohort_map[256] = {
	/* no chicken promoted */
	 0,  1, -1,  2,   3,  4, -1,  5,  -1, -1, -1, -1,   6,  7, -1,  8,
	 9, 10, -1, 11,  12, 13, -1, 14,  -1, -1, -1, -1,  15, 16, -1, 17,
	-1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,
	18, 19, -1, 20,  21, 22, -1, 23,  -1, -1, -1, -1,  24, 25, -1, 26,

	/* CHCK_S promoted */
	-1, 27, -1, 28,  -1, 29, -1, 30,  -1, -1, -1, -1,  -1, 31, -1, 32,
	-1, 33, -1, 34,  -1, 35, -1, 36,  -1, -1, -1, -1,  -1, 37, -1, 38,
	-1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,
	-1, 39, -1, 40,  -1, 41, -1, 42,  -1, -1, -1, -1,  -1, 43, -1, 44,

	/* CHCK_G promoted */
	-1, -1, -1, 45,  -1, -1, -1, 46,  -1, -1, -1, -1,  -1, -1, -1, 47,
	-1, -1, -1, 48,  -1, -1, -1, 49,  -1, -1, -1, -1,  -1, -1, -1, 50,
	-1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,
	-1, -1, -1, 51,  -1, -1, -1, 52,  -1, -1, -1, -1,  -1, -1, -1, 53,

	/* both chicks promoted */
	-1, -1, -1, 54,  -1, -1, -1, 55,  -1, -1, -1, -1,  -1, -1, -1, 56,
	-1, -1, -1, 57,  -1, -1, -1, 58,  -1, -1, -1, -1,  -1, -1, -1, 59,
	-1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,
	-1, -1, -1, 60,  -1, -1, -1, 61,  -1, -1, -1, -1,  -1, -1, -1, 62,
};

/*
 * This table contains information for each cohort.  The following
 * information is stored:
 *
 *  - the chicken promotion bits
 *  - how many of each kind of piece there are
 *  - how large the encoding space for each piece group is
 *  - how many positions this cohort contains (not accounting for lion
 *  - position and ownership)
 *
 * Refer to the content of cohort_map for what number is what cohort.
 */
static const struct cohort_info {
	unsigned char status; /* only promotion bits are set */
	unsigned char pieces[3]; /* 0: chicks, 1: giraffes, 2: elephants */
	unsigned char sizes[3];
	unsigned char padding;
} cohort_info[COHORT_COUNT] = {
	0, 0, 0, 0,  1,  1,  1, 0,
	1, 0, 0, 0, 10,  1,  1, 0,
	2, 0, 0, 0, 45,  1,  1, 0,
	0, 1, 0, 0,  1, 10,  1, 0,
	1, 1, 0, 0, 10,  9,  1, 0,
	2, 1, 0, 0, 45,  8,  1, 0,
	0, 2, 0, 0,  1, 45,  1, 0,
	1, 2, 0, 0, 10, 36,  1, 0,
	2, 2, 0, 0, 45, 28,  1, 0,
	0, 0, 1, 0,  1,  1, 10, 0,
	1, 0, 1, 0, 10,  1,  9, 0,
	2, 0, 1, 0, 45,  1,  8, 0,
	0, 1, 1, 0,  1, 10,  9, 0,
	1, 1, 1, 0, 10,  9,  8, 0,
	2, 1, 1, 0, 45,  8,  7, 0,
	0, 2, 1, 0,  1, 45,  8, 0,
	1, 2, 1, 0, 10, 36,  7, 0,
	2, 2, 1, 0, 45, 28,  6, 0,
	0, 0, 2, 0,  1,  1, 45, 0,
	1, 0, 2, 0, 10,  1, 36, 0,
	2, 0, 2, 0, 45,  1, 28, 0,
	0, 1, 2, 0,  1, 10, 36, 0,
	1, 1, 2, 0, 10,  9, 28, 0,
	2, 1, 2, 0, 45,  8, 21, 0,
	0, 2, 2, 0,  1, 45, 28, 0,
	1, 2, 2, 0, 10, 36, 21, 0,
	2, 2, 2, 0, 45, 28, 15, 0,

	1, 0, 0, 1, 10,  1,  1, 0,
	2, 0, 0, 1, 45,  1,  1, 0,
	1, 1, 0, 1, 10,  9,  1, 0,
	2, 1, 0, 1, 45,  8,  1, 0,
	1, 2, 0, 1, 10, 36,  1, 0,
	2, 2, 0, 1, 45, 28,  1, 0,
	1, 0, 1, 1, 10,  1,  9, 0,
	2, 0, 1, 1, 45,  1,  8, 0,
	1, 1, 1, 1, 10,  9,  8, 0,
	2, 1, 1, 1, 45,  8,  7, 0,
	1, 2, 1, 1, 10, 36,  7, 0,
	2, 2, 1, 1, 45, 28,  6, 0,
	1, 0, 2, 1, 10,  1, 36, 0,
	2, 0, 2, 1, 45,  1, 28, 0,
	1, 1, 2, 1, 10,  9, 28, 0,
	2, 1, 2, 1, 45,  8, 21, 0,
	1, 2, 2, 1, 10, 36, 21, 0,
	2, 2, 2, 1, 45, 28, 15, 0,

	2, 0, 0, 2, 45,  1,  1, 0,
	2, 1, 0, 2, 45,  8,  1, 0,
	2, 2, 0, 2, 45, 28,  1, 0,
	2, 0, 1, 2, 45,  1,  8, 0,
	2, 1, 1, 2, 45,  8,  7, 0,
	2, 2, 1, 2, 45, 28,  6, 0,
	2, 0, 2, 2, 45,  1, 28, 0,
	2, 1, 2, 2, 45,  8, 21, 0,
	2, 2, 2, 2, 45, 28, 15, 0,

	2, 0, 0, 3, 45,  1,  1, 0,
	2, 1, 0, 3, 45,  8,  1, 0,
	2, 2, 0, 3, 45, 28,  1, 0,
	2, 0, 1, 3, 45,  1,  8, 0,
	2, 1, 1, 3, 45,  8,  7, 0,
	2, 2, 1, 3, 45, 28,  6, 0,
	2, 0, 2, 3, 45,  1, 28, 0,
	2, 1, 2, 3, 45,  8, 21, 0,
	2, 2, 2, 3, 45, 28, 15, 0,
};

/*
 * The sente lion has five squares to be on: If the lion is on A, he has
 * already won, so this can't happen.  If he's on B, we can mirror the
 * board.  When he's on C, there is no way to place the Gote lion
 * without it being adjacent to the Sente lion so this isn't possible.
 *
 *     +---+
 *     |AAA|
 *     |BCX|
 *     |BXX|
 *     |BXX|
 *     +---+
 *
 * The Gote lion has up to seven squares.  When he's in the opponents
 * promotion zone A he is either in check (in which case the position is
 * invalid) or has already won.  When he's on B, he is in check by Sente
 * which makes the position invalid.  This leaves 7 + 6 + 5 + 3 + 3 = 24
 * positions for the lions:
 *
 *     +---+ +---+ +---+ +---+ +---+
 *     |XXX| |XXX| |XXX| |XXX| |XBB|
 *     |XXX| |XXX| |XBB| |BBB| |XBL|
 *     |XBB| |BBB| |XBL| |BLB| |XBB|
 *     |AAL| |ALA| |AAA| |AAA| |AAA|
 *     +---+ +---+ +---+ +---+ +---+
 *
 * We also assign codes to lion positions with adjacent lions so we can
 * encode every possible position.  However, we do not store these
 * positions in the table base.
 *
 * This table takes the squares of both lions and returns a number
 * representing this position.  Pairs of lion positions that aren't
 * possible are represented with a -1.  The table contains at index
 * lionpos_map[sente_lion][gote_lion - 3] the value for the particular
 * lion configurations.  It is assumed that lions are not in their
 * opponents promotion zones and that the Sente lion is not on the
 * A file.
 */
static const unsigned char lionpos_map[SQUARE_COUNT - 4][SQUARE_COUNT - 3] = {
	24, 25,  0,   1,  2,  3,   4,  5,  6,  /* C4 */
	26, 27, 28,   7,  8,  9,  10, 11, 12,  /* B4 */
	-1, -1, -1,  -1, -1, -1,  -1, -1, -1,  /* A4 */

	-1, 29, 13,  30, 31, 14,  15, 16, 17,  /* C3 */
	32, -1, 33,  34, 35, 36,  18, 19, 20,  /* B3 */
	-1, -1, -1,  -1, -1, -1,  -1, -1, -1,  /* A3 */

	37, 38, 21,  -1, 39, 22,  40, 41, 23,  /* C2 */
	42, 43, 44,  45, -1, 46,  47, 48, 49,  /* B2 */
};

/*
 * This is the inverse table corresponding to lionpos_map. The lower
 * member of each index indicates the square of the Sente lion, the
 * higher member indicates the square of the Gote lion.
 */
static const unsigned char lionpos_inverse[LIONPOS_TOTAL_COUNT][2] = {
	0,  5,  0,  6,  0,  7,  0,  8,  0,  9,  0, 10,  0, 11,
	1,  6,  1,  7,  1,  8,  1,  9,  1, 10,  1, 11,
	3,  5,  3,  8,  3,  9,  3, 10,  3, 11,
	4,  9,  4, 10,  4, 11,
	6,  5,  6,  8,  6, 11,

	0,  3,  0,  4,
	1,  3,  1,  4,  1,  5,
	3,  4,  3,  6,  3,  7,
	4,  3,  4,  5,  4,  6,  4,  7,  4,  8,
	6,  3,  6,  4,  6,  7,  6,  9,  6, 10,
	7,  3,  7,  4,  7,  5,  7,  6,  7,  8,  7,  9,  7, 10,  7, 11
};

/*
 * The following table encodes pairs of permuted square numbers (see
 * documentation for encode_map) for non-lion pieces into a number.
 * The index into the table has the form pairmap[high - 1] where
 * high is the permuted square number of the piece with the higher
 * permuted square number.  The numbering scheme has been carefully
 * laid out such that one table suffices for all amounts of remaining
 * squares.  This is achieved by arranging the codes such that the
 * first code is the unique code for 2 remaining squares, the next
 * two codes are the remaining two codes for 3 squares, etc.
 * Visualized, the encoding scheme looks like this:
 *
 *     0: ##
 *     1: # #
 *     2:  ##
 *     3: #  #
 *     4:  # #
 *     5:   ##
 *     6: #   #
 *     7:  #  #
 *     8:   # #
 *     9:    ##
 *     ...
 *
 * Note that it is also immediately clear that only the high piece needs
 * to be looked up, we can simply add the low square to the value looked
 * up.  We could also compute the whole code using the formula
 *
 *     high * (high - 1) / 2 + low;
 *
 * but that seems to be slower than looking up the first half.
 *
 * The map could only go to 8 because two squares are always given
 * to the lions and because the high piece can never be on the first
 * square, but we also use it to look up the total possible ways to
 * place two pieces on up to 10 squares, so it goes up to 9.
 */
static const unsigned char pairmap[SQUARE_COUNT - 2] = {
	0, 1, 3, 6, 10, 15, 21, 28, 36, 45,
};

/*
 * This algorithm maps piece locations bijectively onto numbers from
 * 0 to n - 1 where n is the number of positions in the cohort
 * corresponding to the position.  The content of p is destroyed in the
 * process.  Here is a pseudo-code description
 * of the algorithm:
 *
 * encode_map(position p, unsigned cohort):
 *     boardmap <- 0 .. SQUARE_COUNT - 1;
 *     inversemap <- 0 .. SQUARE_COUNT - 1;
 *
 *     code <- lion position code;
 *     remove lion squares;
 *
 *     for each kind of piece (chick, giraffe, elephant):
 *         code *= number of possibilities for this piece kind;
 *         code += number describing piece position
 *         remove squares for current kind of piece;
 *
 *     p->status <- status from cohort
 *
 *     return code;
 *
 * The idea is to keep an array of empty squares in boardmap.  Each time
 * we place a piece on a square we remove that square from boardmap by
 * swapping it with the last piece in boardmap and then decrementing the
 * number of squares.  This is done with the auxillary function
 * remove_square implemented below.
 *
 * At the same time, we also try to figure out what cohort this position
 * is in.  This is done by tracking the number of pieces of each kind in
 * cohortbits and looking up this figure in cohort_map later on.  We
 * need to track which pieces were swapped while encoding to adjust the
 * ownership bits. This is done in the oswap variable.
 */
static void remove_square(unsigned char*, unsigned char*, unsigned, unsigned);

static void
encode_pieces(poscode *pc, struct position *p)
{
	unsigned code, i, squares = SQUARE_COUNT, high, low;
	unsigned oswap = 0, cohortbits = 0;

	unsigned char boardmap[SQUARE_COUNT] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
	}, inversemap[SQUARE_COUNT] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
	};

	/* erase ownership information, leaving square numbers */
	for (i = 0; i < PIECE_COUNT; i++)
		p->pieces[i] &= ~GOTE_PIECE;

	code = lionpos_map[p->pieces[LION_S]][p->pieces[LION_G] - 3];
	assert(code != 0xff);

	if (p->pieces[LION_S] > p->pieces[LION_G]) {
		remove_square(boardmap, inversemap, squares--, p->pieces[LION_S]);
		remove_square(boardmap, inversemap, squares--, p->pieces[LION_G]);
	} else {
		remove_square(boardmap, inversemap, squares--, p->pieces[LION_G]);
		remove_square(boardmap, inversemap, squares--, p->pieces[LION_S]);
	}

	for (i = 0; i < 3; i++)
		if (p->pieces[2 * i + 1] == IN_HAND)
			if (p->pieces[2 * i] == IN_HAND)
				/* no piece to encode */
				;
			else {
				/* encode one piece, no swap */
				cohortbits |= 1 << 2 * i;
				code = code * squares + inversemap[p->pieces[2 * i]];
				remove_square(boardmap, inversemap, squares--, p->pieces[2 * i]);
			}
		else
			if (p->pieces[2 * i] == IN_HAND) {
				/* encode one piece, swap */
				oswap |= 3 << 2 * i;
				cohortbits |= 1 << 2 * i;
				code = code * squares + inversemap[p->pieces[2 * i + 1]];
				remove_square(boardmap, inversemap, squares--, p->pieces[2 * i]);
			} else {
				/* encode two pieces */
				cohortbits |= 3 << 2 * i;
				high = inversemap[p->pieces[2 * i]];
				low  = inversemap[p->pieces[2 * i + 1]];

				/* need swap? */
				if (high < low) {
					oswap |= 3 << 2 * i;
					unsigned tmp = high;
					high = low;
					low = tmp;
				}

				code = code * pairmap[squares] + pairmap[high] + low;
				remove_square(boardmap, inversemap, squares--, high);
				remove_square(boardmap, inversemap, squares--, low);
			}

	/* fix ownership and promotion bits */
	pc->ownership ^= oswap & owner_flip[pc->ownership];

	/* look up cohort */
	cohortbits |= (oswap & 3 ? prom_flip[p->status] : p->status) << 6;
	pc->cohort = cohort_map[cohortbits];
	assert(pc->cohort != (unsigned char)-1);

	pc->map = code;
}

/*
 * swap square sq with the last square on the board with the board where
 * n is the number of squares on the board. This operation is
 * naively
 *
 *    swap inversemap[boardmap[sq]] and inversemap[boardmap[n - 1]]
 *    swap boardmap[sq] and boardmap[n - 1];
 *
 * but we can optimize this procedure by realizing that both boardmap[n]
 * and inversemap[boardmap[n]] are never read again allowing us to skip
 * assigning to them.
 */
static void
remove_square(unsigned char *boardmap, unsigned char *inversemap, unsigned n, unsigned sq)
{
	inversemap[boardmap[n - 1]] = sq;
	boardmap[sq] = boardmap[n - 1];
}

/*
 * This function takes a cohort and a piece map into the cohort and
 * decodes the pieces encoded in there into p.  The order of pieces
 * of the same kind is indeterminate and must be corrected with a call
 * to normalize_position afterwards.
 */
static void
place_pieces(struct position *p, unsigned cohort, unsigned map)
{
	const struct cohort_info *chinfo = cohort_info + cohort;

	unsigned lioncode, code[3], i, squares = SQUARE_COUNT, high, low;

	unsigned char boardmap[SQUARE_COUNT] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
	};

	code[2] = map % chinfo->sizes[2];
	map /= chinfo->sizes[2];
	code[1] = map % chinfo->sizes[1];
	map /= chinfo->sizes[1];
	code[0] = map % chinfo->sizes[0];
	map /= chinfo->sizes[0];
	lioncode = map % LIONPOS_TOTAL_COUNT;

	p->pieces[LION_S] = high = lionpos_inverse[lioncode][0];
	p->pieces[LION_G] = low = lionpos_inverse[lioncode][1];

	if (high > low) {
		boardmap[high] = boardmap[--squares];
		boardmap[low] = boardmap[--squares];
	} else {
		boardmap[low] = boardmap[--squares];
		boardmap[high] = boardmap[--squares];
	}

	for (i = 0; i < 3; i++) {
		switch (chinfo->pieces[i]) {
		case 0:
			continue;

		case 1:
			p->pieces[2 * i] = boardmap[code[i]];
			p->pieces[2 * i + 1] = IN_HAND;
			boardmap[code[i]] = boardmap[--squares];
			break;

		case 2:
			/* find high index */
			for (high = squares - 1; pairmap[high] > code[i]; high--)
				;

			low = code[i] - pairmap[high];
			p->pieces[2 * i] = boardmap[high];
			p->pieces[2 * i + 1] = boardmap[low];
			boardmap[high] = boardmap[--squares];
			boardmap[low] = boardmap[--squares];
			break;

		default:
			/* UNREACHABLE */
			assert(chinfo->pieces[i] <= 2);
		}
	}

	p->status = chinfo->status;
}
