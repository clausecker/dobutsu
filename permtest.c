#include <assert.h>
#include <stdio.h>

#include "dobutsutable.h"

/*
 * This testprogram permutes the contents of the table base and writes
 * them to disk in arbitrary order.  This is useful to find out which
 * permutation yields the smallest database.  The following table
 * contains all permutations (order of loops, OCLM standing for
 * ownership, cohort, lionpos, map) and the result of compressing the
 * permuted files with xz.  Note that map must be inside cohort as the
 * encoding space for map depends on what cohort we are in:
 *
 * xz-compressed results at 049a03fc97dbe3660ef2e2e022137a84cee74cee:
 *
 *     CLMO  45443436  17.80%
 *     LCMO  45714764  17.91%
 *     LOCM  46322540  18.15%
 *     OCLM  46467820  18.20%
 *     LCOM  46529936  18.23%
 *     OLCM  46618092  18.26%
 *     CMLO  47228528  18.50%
 *     COLM  48167068  18.87%
 *     CLOM  48456136  18.98%
 *     OCML  55479520  21.73%
 *     CMOL  56271544  22.04%
 *     COML  57861556  22.67%
 */

extern int
main(int argc, char *argv[])
{
	struct tablebase *tb;
	poscode pc;
	FILE *tbfile;
	size_t size;

	assert(argc == 2);
	tbfile = fopen(argv[1], "rb");
	assert(tbfile != NULL);
	tb = read_tablebase(tbfile);
	assert(tb != NULL);

	for (pc.lionpos = 0; pc.lionpos < LIONPOS_COUNT; pc.lionpos++) {
	for (pc.cohort = 0; pc.cohort < COHORT_COUNT; pc.cohort++) { size = cohort_size[pc.cohort].size;
	for (pc.map = 0; pc.map < size; pc.map++) {
	for (pc.ownership = 0; pc.ownership < OWNERSHIP_COUNT; pc.ownership++) {
		putchar(tb->positions[position_offset(pc)]);
	}}}}

	return (0);
}
