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
	unsigned occupied, overlap, reachable;
	pos_code Ll, op, gp, Gp, ep, Ep, cp, Cp;
	const unsigned char *pos_tab;

	if (pc >= MAX_POS)
		return (POS_INVALID);

	/* unpack fields from code */
	op = pc % 90;
	pc /= 90;
	Ll = pc % 24;
	pc /= 24;
	Gp = pc % 11;
	pc /= 11;
	gp = pc % 11;
	pc /= 11;
	Ep = pc % 11;
	pc /= 11;
	ep = pc % 11;
	pc /= 11;
	Cp = pc % 11;
	pc /= 11;
	cp = pc;

	/* ownership and promotion */
	p->op = op_decoding[op];

	/*
	 * check if pieces overlap. No piece can overlap with the lions
	 * due to how the encoding works.
	 */
	occupied = 1 << Cp;
	overlap = occupied & 1 << cp;
	occupied |= 1 << cp;
	overlap |= occupied & 1 << Ep;
	occupied |= 1 << Ep;
	overlap |= occupied & 1 << ep;
	occupied |= 1 << ep;
	overlap |= occupied & 1 << Gp;
	occupied |= 1 << Gp;
	overlap |= occupied & 1 << gp;

	/* disregard pieces in hand in overlap check */
	if (overlap & 01777)
		return (POS_INVALID);

	/* place lions */
	p->L = lion_decoding[Ll] >> 4;
	p->l = lion_decoding[Ll] & 0xf;

	/*
	 * the encoding only considers the fields the lions don't
	 * occupy. This section turns the encoding back into normal
	 * field numbers.
	 */
	pos_tab = pos_decoding + 11 * Ll;
	p->c = pos_tab[cp];
	p->C = pos_tab[Cp];
	p->e = pos_tab[ep];
	p->E = pos_tab[Ep];
	p->g = pos_tab[gp];
	p->G = pos_tab[Gp];

	/*
	 * check if the gote lion is in check.
	 */
	reachable = 0;
	if (p->op & co) reachable |= p->op & cp ? Rmoves[p->c] : Cmoves[p->c];
	if (p->op & Co) reachable |= p->op & Cp ? Rmoves[p->C] : Cmoves[p->C];
	if (p->op & Eo) reachable |= Eemoves[p->E];
	if (p->op & eo) reachable |= Eemoves[p->e];
	if (p->op & Go) reachable |= Ggmoves[p->G];
	if (p->op & go) reachable |= Ggmoves[p->g];
	if (reachable & 1 << p->l)
		return (POS_SENTE);

	/*
	 * a chick in hand must not be promoted
	 */
	if (p->c == 12 && p->op & cp)
		return (POS_INVALID);

	if (p->C == 12 && p->op & Cp)
		return (POS_INVALID);

	/*
	 * parity check: if both pieces of one kind are owned by the
	 * same party, the first one must not be on a lower field than
	 * the second.
	 */
	if (invariants[op] & CINVARIANT && p->c < p->C)
		return (POS_INVALID);

	if (invariants[op] & EINVARIANT && p->e < p->E)
		return (POS_INVALID);

	if (invariants[op] & GINVARIANT && p->g < p->G)
		return (POS_INVALID);

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
