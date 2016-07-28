#include <assert.h>

#include "dobutsu.h"

/*
 * Perform sanity and mate checks on pos: Return POS_OK if the position
 * is okay, POS_INVALID if it is invalid (e.g. two pieces occupying the
 * same position), POS_SENTE if the gote lion is mated or the sente lion
 * can ascend to the promotion.
 */
extern int
check_pos(const struct position *p)
{
	unsigned occupied, overlap, reach_sente, reach_gote;

	/* the lions may not be in their opponents promotion zones */
	if (07000 & 1 << p->L || 00007 & 1 << p->l)
		return (POS_INVALID);

	/* check if any pieces overlap */
	occupied = 1 << p->L;
	overlap = occupied & 1 << p->l;
	occupied |= 1 << p->l;
	overlap |= occupied & 1 << p->C;
	occupied |= 1 << p->C;
	overlap |= occupied & 1 << p->c;
	occupied |= 1 << p->c;
	overlap |= occupied & 1 << p->E;
	occupied |= 1 << p->E;
	overlap |= occupied & 1 << p->e;
	occupied |= 1 << p->e;
	overlap |= occupied & 1 << p->G;
	occupied |= 1 << p->G;
	overlap |= occupied & 1 << p->g;

	/* disregard pieces in hand in overlap check */
	if (overlap & ALL_SQUARES)
		return (POS_INVALID);

	/* a chick in hand must not be promoted */
	if (p->c == IN_HAND && p->op & cp)
		return (POS_INVALID);

	if (p->C == IN_HAND && p->op & Cp)
		return (POS_INVALID);

	/*
	 * enumerate the squares controled by sente and gote.
	 */
	reach_sente = Llmoves[p->L];
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
		reach_gote |= Ggmoves[p->G];

	if (p->op & go)
		reach_sente |= Ggmoves[p->g];
	else
		reach_gote |= Ggmoves[p->g];

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

	/* if neither apply, the position is normal */
	return (POS_OK);
}

/*
 * Decode pc into p. If pc >= MAX_POS, behaviour is undefined.  If pc
 * does not decode into a valid position, check_pos tells you that this
 * is the case.
 */
extern void
decode_pos(struct position *p, pos_code pc)
{
	pos_code Ll, Gg, Ee, Cc;
	const unsigned char *pos_tab;

	assert(pc < MAX_POS);

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
	pos_tab = pos1_decoding + 11 * Ll;
	p->C = pos_tab[Cc >> 4];
	p->c = pos_tab[Cc & 0xf];
	p->E = pos_tab[Ee >> 4];
	p->e = pos_tab[Ee & 0xf];
	p->G = pos_tab[Gg >> 4];
	p->g = pos_tab[Gg & 0xf];
	p->L = lion_decoding[Ll] >> 4;
	p->l = lion_decoding[Ll] & 0xf;
}

/*
 * Generate the database index for a position. If it is invalid, return
 * POS_INVALID.  If it is valid but cannot be encoded because Sente has
 * a winning position, return POS_SENTE.
 */
extern pos_code
encode_pos(const struct position *pos)
{
	struct position p = *pos;
	unsigned Ll, Ee, Gg, Cc, flip = 0;
	int check = check_pos(pos);
	const signed char *postab;

	if (check != POS_OK)
		return ((pos_code)check);

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
