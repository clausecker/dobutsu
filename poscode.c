#include <assert.h>

#include "dobutsutable.h"

static void	mirror_board(struct position *);
static void	turn_board(struct position *);
static void	normalize_position(struct position *);
static unsigned	encode_ownership(const struct position *);
static void	encode_pieces(poscode *, struct position *);
static void	place_pieces(struct position *, unsigned, unsigned, unsigned);
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

	assert(has_valid_ownership(*pc));

	return (0);
}


/*
 * Decode a tablebase index (poscode) into a position structure.  On
 * success, 0 is returned, in case of failure, -1.  It is assumed that
 * pc is the output of encode_position() for a valid position.
 */
extern int
decode_poscode(struct position *pos, const poscode pc)
{

	place_pieces(pos, pc.cohort, pc.lionpos, pc.map);
	assign_ownership(pos, pc.ownership);
	populate_map(pos);

	return (0);
}

/*
 * Vertically mirror p along the B file.  Do not update p->map.
 */
static void
mirror_board(struct position *p)
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

	for (i = 0; i < PIECE_COUNT; i++)
		p->pieces[i] = flipped_board[p->pieces[i]];
}

/*
 * Turn the board 180 degrees, exchanging Sente and Gote.  Do not update
 * p->map.
 */
static void
turn_board(struct position *p)
{
	size_t i;
	unsigned char tmp;

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

	for (i = 0; i < PIECE_COUNT; i++)
		p->pieces[i] = turned_board[p->pieces[i]];

	/* exchange lions */
	tmp = p->pieces[LION_S];
	p->pieces[LION_S] = p->pieces[LION_G];
	p->pieces[LION_G] = tmp;

	null_move(p);
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

	if (gote_moves(p))
		turn_board(p);

	if (piece_in(00444, p->pieces[LION_S])
	    || piece_in(02222, p->pieces[LION_S]) && piece_in(01111 << GOTE_PIECE, p->pieces[LION_G]))
		mirror_board(p);
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
	p->pieces[LION_G] |= GOTE_PIECE;
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
 * See dobutsutable.h for documentation.
 */
const struct cohort_info cohort_info[COHORT_COUNT] = {
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

/* dito */
const struct cohort_size cohort_size[COHORT_COUNT] = {
	        0,     1,
	     1344,    10,
	    14784,    45,
	    75264,    10,
	    88704,    90,
	   209664,   360,
	   693504,    45,
	   753984,   360,
	  1237824,  1260,
	  2931264,    10,
	  2944704,    90,
	  3065664,   360,
	  3549504,    90,
	  3670464,   720,
	  4638144,  2520,
	  8025024,   360,
	  8508864,  2520,
	 11895744,  7560,
	 22056384,    45,
	 22116864,   360,
	 22600704,  1260,
	 24294144,   360,
	 24777984,  2520,
	 28164864,  7560,
	 38325504,  1260,
	 40018944,  7560,
	 50179584, 18900,
	 75581184,    10,
	 75594624,    45,
	 75655104,    90,
	 75776064,   360,
	 76259904,   360,
	 76743744,  1260,
	 78437184,    90,
	 78558144,   360,
	 79041984,   720,
	 80009664,  2520,
	 83396544,  2520,
	 86783424,  7560,
	 96944064,   360,
	 97427904,  1260,
	 99121344,  2520,
	102508224,  7560,
	112668864,  7560,
	122829504, 18900,
	148231104,    45,
	148291584,   360,
	148775424,  1260,
	150468864,   360,
	150952704,  2520,
	154339584,  7560,
	164500224,  1260,
	166193664,  7560,
	176354304, 18900,
	201755904,    45,
	201816384,   360,
	202300224,  1260,
	203993664,   360,
	204477504,  2520,
	207864384,  7560,
	218025024,  1260,
	219718464,  7560,
	229879104, 18900,
};

/* dito */
const unsigned long long valid_ownership_map[COHORT_COUNT] = {
	0xb0bb0000b0bbb0bbULL,
	0xf0ff0000f0fff0ffULL,
	0xf0ff0000f0fff0ffULL,
	0xbbbb0000bbbbbbbbULL,
	0xffff0000ffffffffULL,
	0xffff0000ffffffffULL,
	0xbbbb0000bbbbbbbbULL,
	0xffff0000ffffffffULL,
	0xffff0000ffffffffULL,
	0xb0bbb0bbb0bbb0bbULL,
	0xf0fff0fff0fff0ffULL,
	0xf0fff0fff0fff0ffULL,
	0xbbbbbbbbbbbbbbbbULL,
	0xffffffffffffffffULL,
	0xffffffffffffffffULL,
	0xbbbbbbbbbbbbbbbbULL,
	0xffffffffffffffffULL,
	0xffffffffffffffffULL,
	0xb0bbb0bbb0bbb0bbULL,
	0xf0fff0fff0fff0ffULL,
	0xf0fff0fff0fff0ffULL,
	0xbbbbbbbbbbbbbbbbULL,
	0xffffffffffffffffULL,
	0xffffffffffffffffULL,
	0xbbbbbbbbbbbbbbbbULL,
	0xffffffffffffffffULL,
	0xffffffffffffffffULL,
	0xf0ff0000f0fff0ffULL,
	0xf0ff0000f0fff0ffULL,
	0xffff0000ffffffffULL,
	0xffff0000ffffffffULL,
	0xffff0000ffffffffULL,
	0xffff0000ffffffffULL,
	0xf0fff0fff0fff0ffULL,
	0xf0fff0fff0fff0ffULL,
	0xffffffffffffffffULL,
	0xffffffffffffffffULL,
	0xffffffffffffffffULL,
	0xffffffffffffffffULL,
	0xf0fff0fff0fff0ffULL,
	0xf0fff0fff0fff0ffULL,
	0xffffffffffffffffULL,
	0xffffffffffffffffULL,
	0xffffffffffffffffULL,
	0xffffffffffffffffULL,
	0xf0ff0000f0fff0ffULL,
	0xffff0000ffffffffULL,
	0xffff0000ffffffffULL,
	0xf0fff0fff0fff0ffULL,
	0xffffffffffffffffULL,
	0xffffffffffffffffULL,
	0xf0fff0fff0fff0ffULL,
	0xffffffffffffffffULL,
	0xffffffffffffffffULL,
	0xf0ff0000f0fff0ffULL,
	0xffff0000ffffffffULL,
	0xffff0000ffffffffULL,
	0xf0fff0fff0fff0ffULL,
	0xffffffffffffffffULL,
	0xffffffffffffffffULL,
	0xf0fff0fff0fff0ffULL,
	0xffffffffffffffffULL,
	0xffffffffffffffffULL,
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
 * invalid) or has already won.  When he's on B, we can mirror the
 * board. When he's on C, he is in check by Sente which makes the
 * position invalid.  This leaves 7 + 4 + 5 + 2 + 3 = 21
 * positions for the lions:
 *
 *     +---+ +---+ +---+ +---+ +---+
 *     |XXX| |XXB| |XXX| |XXB| |XCC|
 *     |XXX| |XXB| |XCC| |CCB| |XCL|
 *     |XCC| |CCB| |XCL| |CLB| |XCC|
 *     |AAL| |ALB| |AAA| |AAB| |AAA|
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
 * opponents promotion zones, that the Sente lion is not on the
 * A file and that if the Sente lion is on the B file, the Gote lion is
 * not on the C file.
 */
static const unsigned char lionpos_map[SQUARE_COUNT - 4][SQUARE_COUNT - 3] = {
	21, 22,  0,   1,  2,  3,   4,  5,  6,  /* C4 */
	-1, 23, 24,  -1,  7,  8,  -1,  9, 10,  /* B4 */
	-1, -1, -1,  -1, -1, -1,  -1, -1, -1,  /* A4 */

	-1, 25, 11,  26, 27, 12,  13, 14, 15,  /* C3 */
	-1, -1, 28,  -1, 29, 30,  -1, 16, 17,  /* B3 */
	-1, -1, -1,  -1, -1, -1,  -1, -1, -1,  /* A3 */

	31, 32, 18,  -1, 33, 19,  34, 35, 20,  /* C2 */
	-1, 36, 37,  -1, -1, 38,  -1, 39, 40,  /* B2 */
};

/*
 * This is the inverse table corresponding to lionpos_map. The lower
 * member of each index indicates the square of the Sente lion, the
 * higher member indicates the square of the Gote lion.
 */
static const unsigned char lionpos_inverse[LIONPOS_TOTAL_COUNT][2] = {
	0,  5,  0,  6,  0,  7,  0,  8,  0,  9,  0, 10,  0, 11,
	1,  7,  1,  8,  1, 10,  1, 11,
	3,  5,  3,  8,  3,  9,  3, 10,  3, 11,
	4, 10,  4, 11,
	6,  5,  6,  8,  6, 11,

	0,  3,  0,  4,
	1,  4,  1,  5,
	3,  4,  3,  6,  3,  7,
	4,  5,  4,  7,  4,  8,
	6,  3,  6,  4,  6,  7,  6,  9,  6, 10,
	7,  4,  7,  5,  7,  8,  7, 10,  7, 11
};

/*
 * The following table encodes pairs of permuted square numbers (see
 * documentation for encode_map) for non-lion pieces into a number.
 * The index into the table has the form pair_map[high - 1] where
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
 *
 * pair_inverse is a lookup table to turn pairs of squares back into
 * square numbers pair_inverse[i] is the largest number such that
 * pair_map[pair_inverse[i]] <= i.
 */
static const unsigned char pair_map[SQUARE_COUNT - 2] = {
	0, 1, 3, 6, 10, 15, 21, 28, 36, 45,
}, pair_inverse[45] = {
	0,
	1, 1,
	2, 2, 2,
	3, 3, 3, 3,
	4, 4, 4, 4, 4,
	5, 5, 5, 5, 5, 5,
	6, 6, 6, 6, 6, 6, 6,
	7, 7, 7, 7, 7, 7, 7, 7,
	8, 8, 8, 8, 8, 8, 8, 8, 8,
};

/*
 * This algorithm maps piece locations bijectively onto numbers from
 * 0 to n - 1 where n is the number of positions in the cohort
 * corresponding to the position.  The content of p is destroyed in the
 * process.  Here is a pseudo-code description
 * of the algorithm:
 *
 * encode_map(position p, unsigned cohort):
 *     board_map <- 0 .. SQUARE_COUNT - 1;
 *     inverse_map <- 0 .. SQUARE_COUNT - 1;
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
 * The idea is to keep an array of empty squares in board_map.  Each time
 * we place a piece on a square we remove that square from board_map by
 * swapping it with the last piece in board_map and then decrementing the
 * number of squares.  This is done with the auxillary function
 * remove_square implemented below.
 *
 * At the same time, we also try to figure out what cohort this position
 * is in.  This is done by tracking the number of pieces of each kind in
 * cohortbits and looking up this figure in cohort_map later on.  We
 * need to track which pieces were swapped while encoding to adjust the
 * ownership bits and if where two both pieces of a kind are in hand,
 * normalize their ownership by swapping them if needed.  This is done in
 * the oswap variable.
 */
static void remove_square(unsigned char*, unsigned char*, unsigned, unsigned);

static void
encode_pieces(poscode *pc, struct position *p)
{
	unsigned code = 0, i, squares = SQUARE_COUNT, high, low;
	unsigned oswap = 0, cohortbits = 0;

	unsigned char board_map[SQUARE_COUNT] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
	}, inverse_map[SQUARE_COUNT] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
	};

	/* erase ownership information, leaving square numbers */
	for (i = 0; i < PIECE_COUNT; i++)
		p->pieces[i] &= ~GOTE_PIECE;

	pc->lionpos = lionpos_map[p->pieces[LION_S]][p->pieces[LION_G] - 3];
	assert(pc->lionpos != 0xff);

	if (p->pieces[LION_S] > p->pieces[LION_G]) {
		remove_square(board_map, inverse_map, --squares, p->pieces[LION_S]);
		remove_square(board_map, inverse_map, --squares, p->pieces[LION_G]);
	} else {
		remove_square(board_map, inverse_map, --squares, p->pieces[LION_G]);
		remove_square(board_map, inverse_map, --squares, p->pieces[LION_S]);
	}

	for (i = 0; i < 6; i += 2)
		if (p->pieces[i + 1] == IN_HAND)
			/* no piece to encode but normalize ownership */
			if (p->pieces[i] == IN_HAND) {
				if ((pc->ownership & 3 << i) == 2U << i)
					oswap |= 3 << i;
			} else {
				/* encode one piece, no swap */
				cohortbits |= 1 << i;
				code = code * squares + inverse_map[p->pieces[i]];
				remove_square(board_map, inverse_map, --squares, p->pieces[i]);
			}
		else
			if (p->pieces[i] == IN_HAND) {
				/* encode one piece, swap */
				oswap |= 3 << i;

				cohortbits |= 1 << i;
				code = code * squares + inverse_map[p->pieces[i + 1]];
				remove_square(board_map, inverse_map, --squares, p->pieces[i + 1]);
			} else {
				/* encode two pieces */
				cohortbits |= 3 << i;
				high = inverse_map[p->pieces[i]];
				low  = inverse_map[p->pieces[i + 1]];

				/* need swap? */
				if (high < low) {
					oswap |= 3 << i;

					unsigned tmp = high;
					high = low;
					low = tmp;
				}

				code = code * pair_map[squares - 1] + pair_map[high - 1] + low;
				remove_square(board_map, inverse_map, --squares, high);
				remove_square(board_map, inverse_map, --squares, low);
			}

	/* fix ownership and promotion bits */
	pc->ownership ^= oswap & owner_flip[pc->ownership];
	if (oswap & 3)
		p->status = prom_flip[p->status];

	/* look up cohort */
	cohortbits |= p->status << 6;
	pc->cohort = cohort_map[cohortbits];
	assert(pc->cohort != (unsigned char)-1);

	pc->map = code;
}

/*
 * swap the square of the piece pc with the last square on the board with the
 * board where n is the last square on the board. This operation is
 * naively
 *
 *    sq = inverse_map[pc],
 *    swap inverse_map[board_map[sq]] and inverse_map[board_map[n]]
 *    swap board_map[sq] and board_map[n];
 *
 * but we can optimize this procedure by realizing that both board_map[n]
 * and inverse_map[pc] are never read again allowing us to skip
 * assigning to them.
 */
static void
remove_square(unsigned char *board_map, unsigned char *inverse_map, unsigned n, unsigned pc)
{
	unsigned sq = inverse_map[pc];

	inverse_map[board_map[n]] = sq;
	board_map[sq] = board_map[n];
}

/*
 * This function takes a cohort and a piece map into the cohort and
 * decodes the pieces encoded in there into p.  The order of pieces
 * of the same kind is indeterminate and must be corrected with a call
 * to normalize_position afterwards.
 */
static void
place_pieces(struct position *p, unsigned cohort, unsigned lionpos, unsigned map)
{
	const struct cohort_info *chinfo = cohort_info + cohort;

	unsigned code[3], i, squares = SQUARE_COUNT, high, low;

	unsigned char board_map[SQUARE_COUNT] = {
		0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
	};

	code[2] = map % chinfo->sizes[2];
	map /= chinfo->sizes[2];
	code[1] = map % chinfo->sizes[1];
	map /= chinfo->sizes[1];
	code[0] = map; /* % chinfo->sizes[0] */

	p->pieces[LION_S] = high = lionpos_inverse[lionpos][0];
	p->pieces[LION_G] = low = lionpos_inverse[lionpos][1];

	if (high > low) {
		board_map[high] = board_map[--squares];
		board_map[low] = board_map[--squares];
	} else {
		board_map[low] = board_map[--squares];
		board_map[high] = board_map[--squares];
	}

	for (i = 0; i < 3; i++) {
		switch (chinfo->pieces[i]) {
		case 0:
			p->pieces[2 * i] = IN_HAND;
			p->pieces[2 * i + 1] = IN_HAND;
			continue;

		case 1:
			p->pieces[2 * i] = board_map[code[i]];
			p->pieces[2 * i + 1] = IN_HAND;
			board_map[code[i]] = board_map[--squares];
			break;

		case 2:
			high = pair_inverse[code[i]];
			low = code[i] - pair_map[high];
			assert(high >= low);

			p->pieces[2 * i] = board_map[high + 1];
			p->pieces[2 * i + 1] = board_map[low];
			board_map[high + 1] = board_map[--squares];
			board_map[low] = board_map[--squares];
			break;

		default:
			/* UNREACHABLE */
			assert(chinfo->pieces[i] <= 2);
		}
	}

	p->status = chinfo->status;
}

/*
 * If the position p can be mirrored such that the result has a
 * different poscode than the original, mirror p and return nonzero.
 * Otherwise return 0 and leave p unchaged.  This function does not
 * update p->map, its intent is to allow for the generation of both
 * poscodes of a given position for generating the table base.  It
 * should not be used for other purposes.
 */
extern int
position_mirror(struct position *p)
{

	/*
	 * We only check if both lions are on the B file.  This is
	 * not as precise as possible, but the gain of doing less
	 * checks for every position probably offsets the loss of
	 * encoding a few more positions than necessary.
	 */
	if (piece_in(02222, p->pieces[LION_S])
	    && piece_in(02222 << GOTE_PIECE, p->pieces[LION_G])) {
		mirror_board(p);
		return (1);
	} else
		return (0);
}

/*
 * This useful auxillary function encodes a position and then looks it
 * up in the table base, saving some time.
 */
extern tb_entry
lookup_position(const struct tablebase *tb, const struct position *p)
{
	poscode pc;

	/* checkmates aren't looked up */
	if (gote_moves(p) ? sente_in_check(p) : gote_in_check(p))
		return (1);

	encode_position(&pc, p);

	return (tb->positions[position_offset(pc)]);
}
