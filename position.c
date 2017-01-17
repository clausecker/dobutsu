#include "dobutsu.h"



/*
 * Check if a and b refer to the same position, return nonzero if and
 * only if they do.  We need this function instead of just memcmp()
 * since the same position can be represented in multiple ways.
 */
extern int
position_equal(const struct position *a, const struct position *b)
{
	size_t i;
	static const unsigned char prom_flip[4] = { 0, 2, 1, 3 };

	if (gote_moves(a) != gote_moves(b))
		return (0);

	/* fast path, should be enough for most positions */
	if (a->map != b->map)
		return (0);

	if ((a->pieces[CHCK_S] != b->pieces[CHCK_S]
	   || a->pieces[CHCK_G] != b->pieces[CHCK_G]
	   || a->status != b->status)
	   && (a->pieces[CHCK_S] != b->pieces[CHCK_G]
	   || a->pieces[CHCK_G] != b->pieces[CHCK_S]
	   || (a->status & 3) != prom_flip[b->status & 3]))
		return (0);

	for (i = 2; i < PIECE_COUNT; i += 2)
		if ((a->pieces[i] != b->pieces[i] || a->pieces[i + 1] != b->pieces[i + 1])
		    && (a->pieces[i] != b->pieces[i + 1] || a->pieces[i + 1] != b->pieces[i]))
			return (0);

	return (1);
}
