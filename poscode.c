#include "dobutsu.h"
#include "postab.c"

/*
 * lookup table for mirroring the board vertically.
 */
static const unsigned char vert_mirror[13] = {
	0x2, 0x1, 0x0,
	0x5, 0x4, 0x3,
	0x8, 0x7, 0x6,
	0xb, 0xa, 0x9,
	0xc
};

/*
 * Decode pc into p.  Return -1 if pc is invalid.  Invalid position
 * codes are those not smaller than MAX_POS and those with one of the
 * invariants violated.  Additionally, if Gote is in check, Sente can
 * ascend to the promotion zone or if Sente is checkmated, this is
 * indicated.  In such situations the position is not stored in the
 * database, saving some space.
 */
extern int
decode_pos(struct position *p, pos_code pc)
{
	unsigned occupied, overlap, reach_sente, reach_gote;
	pos_code Ll, Gg, Ee, Cc, C, c, E, e, G, g;
	const unsigned char *pos_tab;

	if (pc >= MAX_POS)
		return (POS_INVALID);

	/* unpack fields from code */
	p->op = pc % 256;
	pc /= 256;
	Ll = pc % 24;
	pc /= 24;
	Ee = pos2_decoding[pc % 56];
	pc /= 56;
	Gg = pos2_decoding[pc % 56];
	pc /= 56;
	Cc = pos2_decoding[pc];

	/* unpack positions */
	C = Cc >> 4;
	c = Cc & 0xf;
	E = Ee >> 4;
	e = Ee & 0xf;
	G = Gg >> 4;
	g = Gg & 0xf;

	/*
	 * a chick in hand must not be promoted
	 */
	if (c == 10 && p->op & cp)
		return (POS_INVALID);

	if (C == 10 && p->op & Cp)
		return (POS_INVALID);

	/*
	 * check if pieces overlap. No piece can overlap with the lions
	 * due to how the encoding works.
	 */
	occupied = 1 << C;
	overlap = occupied & 1 << c;
	occupied |= 1 << c;
	overlap |= occupied & 1 << E;
	occupied |= 1 << E;
	overlap |= occupied & 1 << e;
	occupied |= 1 << e;
	overlap |= occupied & 1 << G;
	occupied |= 1 << G;
	overlap |= occupied & 1 << g;

	/* disregard pieces in hand in overlap check */
	if (overlap & 01777)
		return (POS_INVALID);

	/*
	 * the encoding only considers the fields the lions don't
	 * occupy. This section turns the encoding back into normal
	 * field numbers.
	 */
	pos_tab = pos1_decoding + 11 * Ll;
	p->c = pos_tab[c];
	p->C = pos_tab[C];
	p->e = pos_tab[e];
	p->E = pos_tab[E];
	p->g = pos_tab[g];
	p->G = pos_tab[G];

	/* place lions */
	p->L = lion_decoding[Ll] >> 4;
	p->l = lion_decoding[Ll] & 0xf;

	/*
	 * enumerate the fields controled by sente and gote.
	 */
	reach_sente = 0; /* by construction, the sente lion won't give check here */
	reach_gote = Llmoves[p->l];
	if (p->op & Co)
		reach_sente |= p->op & Cp ? Rmoves[p->C] : Cmoves[p->C];
	else
		reach_gote |= p->op & Cp ? rmoves[p->C] : cmoves[p->C];

	if (p->op & co)
		reach_sente |= p->op & cp ? Rmoves[p->c] : Cmoves[p->c];
	else
		reach_gote |= p->op & cp ? rmoves[p->c] : cmoves[p->c];

	if (p->op & Eo)
		reach_sente |= Eemoves[p->E];
	else
		reach_gote |= Eemoves[p->E];

	if (p->op & eo)
		reach_sente |= Eemoves[p->e];
	else
		reach_gote |= Eemoves[p->e];

	if (p->op & Go)
		reach_sente |= Ggmoves[p->G];
	else
		reach_gote |= Eemoves[p->G];

	if (p->op & go)
		reach_sente |= Ggmoves[p->g];
	else
		reach_gote |= Eemoves[p->g];

	/*
	 * check if the gote lion is in check.
	 */
	if (reach_sente & 1 << p->l)
		return (POS_SENTE);

	/*
	 * Check if the sente lion can enter the promotion zone. There is
	 * only one field the lion can be on when he enters the zone.
	 */
	if (p->L == 6 && (reach_gote & 03000) != 03000)
		return (POS_SENTE);

	/*
	 * Check if we are mated.
	 */
	if (reach_gote & 1 << p->L && (~reach_gote & Llmoves[p->L]) == 0)
		return (POS_GOTE);	

	return (POS_OK);
}

/*
 * Generate the database index for a position. If it is invalid, return
 * POS_INVALID.  If it is valid but cannot be encoded, return either
 * POS_SENTE or POS_GOTE.
 */
extern pos_code
encode_pos(const struct position *pos)
{
	struct position p = *pos;
	unsigned Ll, Ee, Gg, Cc;
	const signed char *postab;

	unsigned overlap, occupied, flip = 0;

	/* normalize indices */
	if (p.C > 12)
		p.C = 12;
	if (p.c > 12)
		p.c = 12;
	if (p.E > 12)
		p.E = 12;
	if (p.e > 12)
		p.e = 12;
	if (p.G > 12)
		p.G = 12;
	if (p.g > 12)
		p.g = 12;

	/* first, check if any pieces overlap */
	occupied = 1 << p.L;
	overlap = occupied & 1 << p.l;
	occupied |= 1 << p.l;
	overlap |= occupied & 1 << p.C;
	occupied |= 1 << p.C;
	overlap |= occupied & 1 << p.c;
	occupied |= 1 << p.c;
	overlap |= occupied & 1 << p.E;
	occupied |= 1 << p.E;
	overlap |= occupied & 1 << p.e;
	occupied |= 1 << p.e;
	overlap |= occupied & 1 << p.G;
	occupied |= 1 << p.G;
	overlap |= occupied & 1 << p.g;

	/* disregard pieces in hand in overlap check */
	if (overlap & 07777)
		return (POS_INVALID);

	/* check if Sente is in the promotion zone */
	if (07000 & 1 << p.L)
		return (POS_SENTE);

	/* check if the Sente lion gives check to the Gote lion */
	if (Llmoves[p.L] & 1 << p.l)
		return (POS_SENTE);

	/* check if Gote is in the promotion zone */
	if (00007 & 1 << p.l)
		return (POS_GOTE);

	/* if the Sente lion is on field 2, 5, or 8, flip the board */
	if (00444 & 1 << p.L) {
		p.l = vert_mirror[p.l];
		p.L = vert_mirror[p.L];
		p.c = vert_mirror[p.c];
		p.C = vert_mirror[p.C];
		p.e = vert_mirror[p.e];
		p.E = vert_mirror[p.E];
		p.g = vert_mirror[p.g];
		p.G = vert_mirror[p.G];
	}

	/* now the lion position can be encoded */
	Ll = lion_encoding[8 * p.L + (p.l - 5)];

	/* flip bits in op byte where needed */
	flip = 0;
	if (p.C < p.c)
		flip |= co | cp | Co | Cp;

	if (p.E < p.e)
		flip |= eo | Eo;

	if (p.G < p.g)
		flip |= go | Go;

	p.op ^= flip_op[p.op] & flip;

	/* encode non-lion pieces */
	postab = pos1_encoding + Ll * 13;

	Cc = pos2_encoding[postab[p.c] * 11 + postab[p.C]];
	Ee = pos2_encoding[postab[p.e] * 11 + postab[p.E]];
	Gg = pos2_encoding[postab[p.g] * 11 + postab[p.G]];

	return ((((Cc * 56 + Gg) * 56 + Ee) * 24 + Ll) * 256 + p.op);
}
