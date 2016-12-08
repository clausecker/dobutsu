#include <assert.h>

#include "dobutsutable.h"

static void	normalize_position(struct position *);
static unsigned	encode_ownership(const struct position *);
static unsigned	encode_cohort(const struct position *);
static unsigned	encode_map(struct position *, unsigned);
static void	place_pieces(struct position *, unsigned, unsigned);
static void	decode_ownership(struct position *, unsigned);
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
	pc->cohort = encode_cohort(&p);
	pc->map = encode_map(&p, pc->cohort);

	return (0);
}


/*
 * Decode a tablebase index (poscode) into a position structure.  On
 * success, 0 is returned, in case of failure, -1.  It is assumed that
 * pc is the output of encode_position() for a valid position.
 */
extern int
decode_position(struct position *pos, const poscode *pc)
{

	place_pieces(pos, pc->cohort, pc->map);
	normalize_position(pos);
	assign_ownership(pos, pc->ownership);

	return (0);
}

/*
 * Normalize p, that means:
 *  - if it's Gote to move, turn the board 180 degrees.
 *  - if the Sente lion is on the right board-half, flip the board
 *    along the center file
 *  - if an _S piece is on a higher square than the corresponding _G
 *  - piece, flip the two pieces.  The corresponding promotion bits
 *  - are updated, too.
 */
static void
normalize_position(struct position *p)
{
	size_t i;
	unsigned char tmp;
	static const unsigned char flipped_promotion[4] = { 0, 2, 1, 3 };
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

	if ((p->pieces[CHCK_S] & ~GOTE_PIECE) > (p->pieces[CHCK_G] & ~GOTE_PIECE)) {
		tmp = p->pieces[CHCK_S];
		p->pieces[CHCK_S] = p->pieces[CHCK_G];
		p->pieces[CHCK_G] = tmp;
		p->status = flipped_promotion[p->status];
	}

	if ((p->pieces[GIRA_S] & ~GOTE_PIECE) > (p->pieces[GIRA_G] & ~GOTE_PIECE)) {
		tmp = p->pieces[GIRA_S];
		p->pieces[GIRA_S] = p->pieces[GIRA_G];
		p->pieces[GIRA_G] = tmp;
	}

	if ((p->pieces[ELPH_S] & ~GOTE_PIECE) > (p->pieces[ELPH_G] & ~GOTE_PIECE)) {
		tmp = p->pieces[ELPH_S];
		p->pieces[ELPH_S] = p->pieces[ELPH_G];
		p->pieces[ELPH_G] = tmp;
	}
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
 *  - how many positions this cohort contains (not accounting for lion
 *  - position and ownership)
 *
 * Refer to the content of cohort_map for what number is what cohort.
 */
static const struct cohort_info {
	unsigned char status; /* only promotion bits are set */
	unsigned char pieces[3]; /* 0: chicks, 1: giraffes, 2: elephants */
	unsigned size;
} cohort_info[COHORT_COUNT] = {
	0, 0, 0, 0,  1 *  1 *  1,
	1, 0, 0, 0, 10 *  1 *  1,
	2, 0, 0, 0, 45 *  1 *  1,
	0, 1, 0, 0,  1 * 10 *  1,
	1, 1, 0, 0, 10 *  9 *  1,
	2, 1, 0, 0, 45 *  8 *  1,
	0, 2, 0, 0,  1 * 45 *  1,
	1, 2, 0, 0, 10 * 36 *  1,
	2, 2, 0, 0, 45 * 28 *  1,
	0, 0, 1, 0,  1 *  1 * 10,
	1, 0, 1, 0, 10 *  1 *  9,
	2, 0, 1, 0, 45 *  1 *  8,
	0, 1, 1, 0,  1 * 10 *  9,
	1, 1, 1, 0, 10 *  9 *  8,
	2, 1, 1, 0, 45 *  8 *  7,
	0, 2, 1, 0,  1 * 45 *  8,
	1, 2, 1, 0, 10 * 36 *  7,
	2, 2, 1, 0, 45 * 28 *  6,
	0, 0, 2, 0,  1 *  1 * 45,
	1, 0, 2, 0, 10 *  1 * 36,
	2, 0, 2, 0, 45 *  1 * 28,
	0, 1, 2, 0,  1 * 10 * 36,
	1, 1, 2, 0, 10 *  9 * 28,
	2, 1, 2, 0, 45 *  8 * 21,
	0, 2, 2, 0,  1 * 45 * 28,
	1, 2, 2, 0, 10 * 36 * 21,
	2, 2, 2, 0, 45 * 28 * 15,

	1, 0, 0, 1, 10 *  1 *  1,
	2, 0, 0, 1, 45 *  1 *  1,
	1, 1, 0, 1, 10 *  9 *  1,
	2, 1, 0, 1, 45 *  8 *  1,
	1, 2, 0, 1, 10 * 36 *  1,
	2, 2, 0, 1, 45 * 28 *  1,
	1, 0, 1, 1, 10 *  1 *  9,
	2, 0, 1, 1, 45 *  1 *  8,
	1, 1, 1, 1, 10 *  9 *  8,
	2, 1, 1, 1, 45 *  8 *  7,
	1, 2, 1, 1, 10 * 36 *  7,
	2, 2, 1, 1, 45 * 28 *  6,
	1, 0, 2, 1, 10 *  1 * 36,
	2, 0, 2, 1, 45 *  1 * 28,
	1, 1, 2, 1, 10 *  9 * 28,
	2, 1, 2, 1, 45 *  8 * 21,
	1, 2, 2, 1, 10 * 36 * 21,
	2, 2, 2, 1, 45 * 28 * 15,

	2, 0, 0, 2, 45 *  1 *  1,
	2, 1, 0, 2, 45 *  8 *  1,
	2, 2, 0, 2, 45 * 28 *  1,
	2, 0, 1, 2, 45 *  1 *  8,
	2, 1, 1, 2, 45 *  8 *  7,
	2, 2, 1, 2, 45 * 28 *  6,
	2, 0, 2, 2, 45 *  1 * 28,
	2, 1, 2, 2, 45 *  8 * 21,
	2, 2, 2, 2, 45 * 28 * 15,

	2, 0, 0, 3, 45 *  1 *  1,
	2, 1, 0, 3, 45 *  8 *  1,
	2, 2, 0, 3, 45 * 28 *  1,
	2, 0, 1, 3, 45 *  1 *  8,
	2, 1, 1, 3, 45 *  8 *  7,
	2, 2, 1, 3, 45 * 28 *  6,
	2, 0, 2, 3, 45 *  1 * 28,
	2, 1, 2, 3, 45 *  8 * 21,
	2, 2, 2, 3, 45 * 28 * 15,
};

/*
 * Encode which pieces are on the board into a number between
 * 0 and 81 using the table cohort_map.  It is assumed that
 * the position has been normalized before.
 */
static unsigned
encode_cohort(const struct position *p)
{
	unsigned cohort = 0;

	if (piece_in(BOARD, p->pieces[CHCK_S]))
		cohort |= 1 << 0;
	if (piece_in(BOARD, p->pieces[CHCK_G]))
		cohort |= 1 << 1;
	if (piece_in(BOARD, p->pieces[GIRA_S]))
		cohort |= 1 << 2;
	if (piece_in(BOARD, p->pieces[GIRA_G]))
		cohort |= 1 << 3;
	if (piece_in(BOARD, p->pieces[ELPH_S]))
		cohort |= 1 << 4;
	if (piece_in(BOARD, p->pieces[ELPH_G]))
		cohort |= 1 << 5;
	if (is_promoted(CHCK_S, p))
		cohort |= 1 << 6;
	if (is_promoted(CHCK_G, p))
		cohort |= 1 << 7;

	assert(cohort_map[cohort] != 0xff);

	return (cohort_map[cohort]);
}

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
 */
static void remove_square(unsigned char*, unsigned char*, unsigned, unsigned);

static unsigned
encode_map(struct position *p, unsigned cohort)
{
	const struct cohort_info *chinfo = cohort_info + cohort;

	unsigned code, i, squares = SQUARE_COUNT, high, low;

	unsigned char boardmap[SQUARE_COUNT] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
	}, inversemap[SQUARE_COUNT] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
	};


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

	for (i = 0; i < 3; i++) {
		if (chinfo->pieces[i] == 0)
			continue;

		if (chinfo->pieces[i] == 1) {
			code = code * squares + inversemap[p->pieces[2 * i]];
			remove_square(boardmap, inversemap, squares--, p->pieces[2 * i]);
		} else { /* chinfo->pieces[i] == 2 */
			high = inversemap[p->pieces[2 * i]];
			low  = inversemap[p->pieces[2 * i + 1]];
			if (high < low) {
				unsigned tmp = high;
				high = low;
				low = tmp;
			}

			code = code * pairmap[squares] + pairmap[high] + low;
			remove_square(boardmap, inversemap, squares--, high);
			remove_square(boardmap, inversemap, squares--, low);
		}
	}

	p->status = chinfo->status;

	return (code);
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

	unsigned code, i, squares = SQUARE_COUNT, high, low;

	unsigned char boardmap[SQUARE_COUNT] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
	};

	code = map % LIONPOS_TOTAL_COUNT;
	map /= LIONPOS_TOTAL_COUNT;

	p->pieces[LION_S] = high = lionpos_inverse[code][0];
	p->pieces[LION_G] = low = lionpos_inverse[code][1];

	if (high > low) {
		boardmap[high] = boardmap[--squares];
		boardmap[low] = boardmap[--squares];
	} else {
		boardmap[low] = boardmap[--squares];
		boardmap[high] = boardmap[--squares];
	}

	for (i = 0; i < 3; i++) {
		if (chinfo->pieces[i] == 0)
			continue;

		if (chinfo->pieces[i] == 1) {
			code = map % squares;
			map /= squares;

			p->pieces[2 * i] = boardmap[code];
			p->pieces[2 * i + 1] = IN_HAND;
			boardmap[code] = boardmap[--squares];
		} else { /* chinfo->pieces[i] == 2 */
			code = map % pairmap[squares];
			map /= pairmap[squares];

			/* find high index */
			for (high = squares - 1; pairmap[high] > code; high--)
				;

			low = code - pairmap[high];
			p->pieces[2 * i] = boardmap[high];
			p->pieces[2 * i + 1] = boardmap[low];
			boardmap[high] = boardmap[--squares];
			boardmap[low] = boardmap[--squares];
		}
	}

	p->status = chinfo->status;
}
