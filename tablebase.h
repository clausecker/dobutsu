#ifndef TABLEBASE_H
#define TABLEBASE_H

/*
 * This header contains functionality to generate and probe the
 * comprehensive tablebase.  This header contains a generic interface
 * that isn't specific to a particular tablebase implementation, so
 * the implementation can be swapped with a more suitable one as needed.
 */

#include <stdio.h>

#include "rules.h"

/*
 * A tablebase entry.  The number has the following meaning:
 *  - a positive number indicates a winning position with the number
 *    itself being the distance to ascension/lion capture in double
 *    moves. If ascension/lion capture is possible in this move, the
 *    number is 1.
 *  - zero indicates a draw.
 *  - a negative number indicates a lost position with the number
 *    itself being the distance to ascension/lion capture in double
 *    moves. If the opponent can capture your lion or ascend in his
 *    next move regardless of what you do, the number is 1.
 */
typedef int tb_entry;

/*
 * The tablebase itself.  This structure is opaque.
 */
struct tablebase;

/* tablebase functionality */
extern		struct tablebase	*generate_tablebase(void);
extern		struct tablebase	*read_tablebase(FILE*);
extern		tb_entry		 lookup_position(const struct tablebase*, const struct position*);
extern		int			 write_tablebase(FILE*, const struct tablebase*);
extern		int			 validate_tablebase(const struct tablebase*);
extern		void			 free_tablebase(struct tablebase*);

/* auxillary functionality */
static inline	int			 is_win(tb_entry);
static inline	int			 is_draw(tb_entry);
static inline	int			 is_loss(tb_entry);
static inline	int			 get_dtm(tb_entry);
extern		int			 order_by_wdl(const void*, const void*); /* for qsort */

/* implementations of inline functions */
static inline int
is_win(tb_entry e)
{

	return (e > 0);
}

static inline int
is_draw(tb_entry e)
{

	return (e == 0);
}

static inline int
is_loss(tb_entry e)
{

	return (e < 0);
}

/*
 * Return the distance to mate in half moves.  If e is a draw, the
 * result is unspecified.
 */
static inline int
get_dtm(tb_entry e)
{

	if (is_win(e))
		return (2 * e - 1);
	else
		return (- 2 * e);
}

#endif /* TABLEBASE_H */
