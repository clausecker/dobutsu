#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "dobutsutable.h"

/*
 * Release all storage associated with tb.  The pointer to tb then
 * becomes invalid.
 */
extern void
free_tablebase(struct tablebase *tb)
{
	free(tb);
}

/*
 * Lookup the position with position code pc in the table base.  It is
 * assumed that pc is a valid position code.
 */
extern tb_entry
lookup_poscode(const struct tablebase *tb, poscode pc)
{

	/*
	 * all positions between LIONPOS_COUNT and LIONPOS_TOTAL_COUNT
	 * are immediate wins and not stored in the table base.
	 */
	if (pc.lionpos >= LIONPOS_COUNT)
		return (1);
	else
		return (tb->positions[position_offset(pc)]);
}

/*
 * Write tb to file f.  It is assumed that f has been opened in binary
 * mode and truncated.  This function returns 0 on success, -1 on error
 * with errno indicating the reason for failure.
 */
extern int
write_tablebase(FILE *f, const struct tablebase *tb)
{
	poscode pc;

	rewind(f);

	pc.map = 0;
	for (pc.lionpos = 0; pc.lionpos < LIONPOS_COUNT; pc.lionpos++)
		for (pc.cohort = 0; pc.cohort < COHORT_COUNT; pc.cohort++)
			for (pc.ownership = 0; pc.ownership < OWNERSHIP_COUNT; pc.ownership++)
				fwrite(tb->positions + position_offset(pc),
				    1, cohort_size[pc.cohort].size, f);

	fflush(f);

	return (ferror(f) ? -1 : 0);
}
