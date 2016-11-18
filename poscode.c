#include <assert.h>

#include "dobutsutable.h"

static void	normalize_positions(struct position *);
static unsigned	encode_ownership(const struct position *);
static unsigned	encode_lionpos(const struct position *);
static unsigned	encode_cohort(const struct position *);
static unsigned	encode_map(const struct position *, unsigned);
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
	pc->lionpos = encode_lionpos(&p);
	pc->cohort = encode_cohort(&p);
	pc->map = encode_map(&p, pc->lionpos);

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

	place_pieces(pos, pc->lionpos, pc->cohort, pc->map);
	assign_ownership(pos, pc->ownership);

	return (0);
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
 * the _S piece is on the board, then the _G piece is on the board, too.
 * Furthermore, if both chicks are on the board and CHCK_S is promoted,
 * then CHCK_G is promoted, too.  Entries not corresponding to any
 * cohort are marked -1 (0xff).
 */
static unsigned char cohort_map[256] = {
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

	/* CHCK_G promoted -- invalid */
	-1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,
	-1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,
	-1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,
	-1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,

	/* both chicks promoted */
	-1, -1, -1, 45,  -1, -1, -1, 46,  -1, -1, -1, -1,  -1, -1, -1, 47,
	-1, -1, -1, 48,  -1, -1, -1, 49,  -1, -1, -1, -1,  -1, -1, -1, 50,
	-1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,  -1, -1, -1, -1,
	-1, -1, -1, 51,  -1, -1, -1, 52,  -1, -1, -1, -1,  -1, -1, -1, 53,
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
