#include <stdio.h>

#include "dobutsutable.h"

/*
 * This program test-generates a tablebase with only wins (1),
 * checkmates (-1) and undecided positions (0) filled in to find the
 * ideal permutation of the fields that make up a position.
 */

static void generate_cohort(unsigned, unsigned, unsigned);

extern int
main()
{
	unsigned i, j, k;

	for (k = 0; k < LIONPOS_COUNT; k++)
	for (j = 0; j < COHORT_COUNT; j++)
	for (i = 0; i < OWNERSHIP_COUNT; i++)
		generate_cohort(i, j, k);

	return (0);
}

static void
generate_cohort(unsigned os, unsigned ch, unsigned lp)
{
	struct move moves[MAX_MOVES];
	struct position p, pmoved;
	poscode pc;
	unsigned map, size, i, j, nmove;
	unsigned char outbuf[45 * 28 * 15];

	pc.ownership = os;
	pc.cohort = ch;
	pc.lionpos = lp;

	for (i = 0; i < cohort_size[ch].size; i++) {
		pc.map = i;
		decode_poscode(&p, pc);
		if (gote_in_check(&p)) {
			outbuf[i] = 1;
			continue;
		}

		nmove = generate_moves(moves, &p);
		for (j = 0; j < nmove; j++) {
			pmoved = p;
			play_move(&pmoved, moves[j]);
			if (!sente_in_check(&pmoved))
				goto no_checkmate;
		}

		/* all moves lead to check, we can't escape checkmate */
		outbuf[i] = -1;
		continue;

	no_checkmate:
		outbuf[i] = 0;
	}

	fwrite(outbuf, 1, size, stdout);
}
