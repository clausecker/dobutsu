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
 * A poscode (position code) is an encoded position directly suitable as
 * an index into the endgame tablebase.  A typedef is provided so we can
 * easily switch to an implementation where poscode is a numeric type.
 *
 * The poscode comprises the following pieces:
 *  - ownership is a bitmap indicating who owns which piece
 *  - lionpos is a number indicating what squares the lions are on
 *  - cohort is a number indicating what pieces are on the board and if
 *    they are promoted.
 *  - map is a number indicating which of the pieces occupy what
 *    squares
 *
 * Not all positions are stored in the tablebase: positions with both
 * lions adjacent or one lion already ascended aren't.  Neverthless,
 * even these positions are encodeable for simplicity.
 *
 * Before a position is encoded, it is normalized.  This means:
 *  - if it's Gote's move, the board is turned so a position with Sente
 *    to move obtains.
 *  - the board is flipped horizontally under certain conditions
 *  - pieces of the same kind may be interchanged
 */
typedef struct {
	unsigned ownership;
	unsigned lionpos;
	unsigned cohort;
	unsigned map;
} poscode;

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

/* generating, reading, writing, and closeing the tablebase */
extern		struct tablebase	*generate_tablebase(void);
extern		struct tablebase	*read_tablebase(FILE*);
extern		int			 write_tablebase(FILE*, const struct tablebase*);
extern		void			 free_tablebase(struct tablebase*);

/* lookup functionality */
extern		int			 encode_position(poscode*, const struct position*);
extern		int			 decode_poscode(struct position*, const poscode*);
extern		tb_entry		 lookup_poscode(const struct tablebase*, const poscode);

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
