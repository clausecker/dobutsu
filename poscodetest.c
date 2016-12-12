/*
 * This program decodes every possible poscode and then encodes the
 * resulting position, comparing if the roundtrip results in the
 * original code.  It also counts how many positions there are.
 */

#include <stdio.h>
#include <stdlib.h>

#include "dobutsutable.h"

static unsigned eval_cohort(size_t, unsigned);
static unsigned eval_position(poscode pc);

extern int
main() {
	unsigned cohort, cohortlen, total = 0, totalcheckmates = 0, checkmates;
	const unsigned char *sizes;

	for (cohort = 0; cohort < COHORT_COUNT; cohort++) {
		sizes = cohort_info[cohort].sizes;
		cohortlen = sizes[0] * sizes[1] * sizes[2];
		checkmates = eval_cohort(cohort, cohortlen);

		cohortlen *= LIONPOS_COUNT * OWNERSHIP_COUNT;
		total += cohortlen;
		totalcheckmates += checkmates;

		printf("%2u     %9u  %9u  %.2f%%\n", cohort, cohortlen, checkmates, 100.0 * checkmates / cohortlen);
	}

	printf("total  %9u  %9u  %.2f%%\n", total, totalcheckmates, 100.0 * totalcheckmates / total);
	return (EXIT_SUCCESS);
}

static unsigned
eval_cohort(size_t cohort, unsigned len)
{
	poscode pc;
	unsigned checkmates = 0;

	pc.cohort = cohort;

	for (pc.ownership = 0; pc.ownership < OWNERSHIP_COUNT; pc.ownership++)
		for (pc.lionpos = 0; pc.lionpos < LIONPOS_COUNT; pc.lionpos++)
			for (pc.map = 0; pc.map < len; pc.map++)
				checkmates += eval_position(pc);

	return (checkmates);
}

static unsigned
eval_position(poscode pc)
{
	struct position p;
	poscode newpc;
	char posstr[MAX_POSSTR];

	decode_poscode(&p, &pc);

	if (!position_valid(&p)) {
		printf("(%u, %u, %u, %u) => invalid\n",
		    pc.ownership, pc.cohort, pc.lionpos, pc.map);
		return (0);
	}

	encode_position(&newpc, &p);
	if (newpc.ownership != pc.ownership || newpc.cohort != pc.cohort
	    || newpc.lionpos != pc.lionpos || newpc.map != pc.map) {
		position_string(posstr, &p);
		printf("(%u, %u, %u, %u) => %s => (%u, %u, %u, %u)\n",
		    pc.ownership, pc.cohort, pc.lionpos, pc.map,
		    posstr,
		    newpc.ownership, newpc.cohort, newpc.lionpos, newpc.map);
	}

	return (gote_in_check(&p));
}
