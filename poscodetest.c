/*
 * This program decodes every possible poscode and then encodes the
 * resulting position, comparing if the roundtrip results in the
 * original code.  It also counts how many positions there are.
 */

#include <stdio.h>
#include <stdlib.h>

#include "dobutsutable.h"

static void eval_cohort(size_t, unsigned);

extern int
main() {
	unsigned cohort, cohortlen, total = 0;
	const unsigned char *sizes;

	for (cohort = 0; cohort < COHORT_COUNT; cohort++) {
		sizes = cohort_info[cohort].sizes;
		cohortlen = sizes[0] * sizes[1] * sizes[2] * LIONPOS_TOTAL_COUNT;
		total += cohortlen;

		printf("%2u    %9u\n", cohort, cohortlen * OWNERSHIP_COUNT);
		eval_cohort(cohort, cohortlen);
	}

	printf("total %9u\n", total * OWNERSHIP_COUNT);
	printf("table %9u\n", total * OWNERSHIP_COUNT / LIONPOS_TOTAL_COUNT * LIONPOS_COUNT);

	return (EXIT_SUCCESS);
}

static void
eval_cohort(size_t cohort, unsigned len)
{
	poscode pc, newpc;
	struct position p;
	unsigned ownership, map;
	char posstr[MAX_POSSTR];

	pc.cohort = cohort;

	for (ownership = 0; ownership < OWNERSHIP_COUNT; ownership++) {
		pc.ownership = ownership;
		for (map = 0; map < len; map++) {
			pc.map = map;
			decode_poscode(&p, &pc);
			if (!position_valid(&p)) {
				printf("(%u, %u, %u) => invalid\n", pc.ownership, pc.cohort, pc.map);
				continue;
			}

			encode_position(&newpc, &p);
			if (newpc.ownership != pc.ownership || newpc.cohort != pc.cohort || newpc.map != pc.map) {
				position_string(posstr, &p);
				printf("(%u, %u, %u) => %s => (%u, %u, %u)\n",
				    pc.ownership, pc.cohort, pc.map,
				    posstr,
				    newpc.ownership, newpc.cohort, newpc.map);
			}
		}
	}
}
