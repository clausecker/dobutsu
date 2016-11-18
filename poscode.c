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
