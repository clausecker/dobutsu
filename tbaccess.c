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
 * This useful auxillary function encodes a position and then looks it
 * up in the table base, saving some time.
 */
extern tb_entry
lookup_position(const struct tablebase *tb, const struct position *p)
{
	poscode pc;

	/* checkmates aren't looked up */
	if (gote_moves(p) ? sente_in_check(p) : gote_in_check(p))
		return (1);

	encode_position(&pc, p);

	return (tb->positions[position_offset(pc)]);
}

/*
 * Read a tablebase from file f.  It is assumed that f has been opened
 * in binary mode for reading.  This function returns a pointer to the
 * newly loaded tablebase on success or NULL on error with errno
 * indicating the reason for failure.
 */
extern struct tablebase *
read_tablebase(FILE *f)
{
	struct tablebase *tb = malloc(sizeof *tb);

	if (tb == NULL)
		return (NULL);

	if (fread((void*)tb->positions, sizeof tb->positions, 1, f) != 1) {
		free(tb);
		return (NULL);
	}

	return (tb);
}
