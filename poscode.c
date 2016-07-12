#include "dobutsu.h"

#include "postab.c"

/*
 * Decode pc into p.  Return -1 if pc is invalid.  Invalid position
 * codes are those not smaller than MAX_POS, those were the gote lion is
 * in check (not allowed since it's sente's turn) and those were two
 * pieces try to occupy the same field.
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
	Cc = pos2_decoding[pc % 56];
	pc /= 56;

	/* unpack positions */
	C = Cc & 0xf;
	c = Cc >> 4;
	E = Ee & 0xf;
	e = Ee >> 4;
	G = Gg & 0xf;
	g = Gg >> 4;

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
 * Normalize a position so that it can be found in the database. If it
 * is invalid, return -1.
 */
static int
normalize_pos(struct position *p)
{
	/* ... */
}
