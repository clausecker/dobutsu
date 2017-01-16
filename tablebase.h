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

/*
 * This structure is used by analyze_position() to store the analysis
 * of the moves from a position.  entry indicates what the value of the
 * current position would have been if move was the best move, value
 * indicates the move value as seen by the engine at the selected
 * strength.  0 <= value < 1 holds.
 */
struct analysis {
	struct move move;
	tb_entry entry;
	double value;
};

/*
 * This structure is used to seed the random number generator for the
 * ai.  Internally, the POSIX rand48 random number generator is used
 * with this structure forming a thin wrapper over it.
 */
struct seed {
	unsigned short xsubi[3];
};

enum {
	/*
	 * The maximum number of threads allowed for generate_tablebase.
	 * This limit is arbitrary and can be increased if desired.  The
	 * intent is to avoid crashing people's computers if they
	 * accidentally try to run gentb with 1000 threads.
	 */
	GENTB_MAX_THREADS = 64,

	/*
	 * The last parameter to ai_move() indicates the ai strength,
	 * which should be an integer between 0 and MAX_STRENGTH.  This
	 * limit has been set with some safety margin such that no
	 * floating point overflow happens during computation.
	 */
	MAX_STRENGTH = 700,
};

/* tablebase functionality */
extern		struct tablebase	*generate_tablebase(int);
extern		struct tablebase	*read_tablebase(FILE*);
extern		tb_entry		 lookup_position(const struct tablebase*, const struct position*);
extern		int			 write_tablebase(FILE*, const struct tablebase*);
extern		int			 validate_tablebase(const struct tablebase*);
extern		void			 free_tablebase(struct tablebase*);

/* ai functionality */
extern		void			 ai_seed(struct seed*);
extern		struct move		 ai_move(const struct tablebase*, const struct position*,
					     struct seed*, double);
extern		size_t			 analyze_position(struct analysis[MAX_MOVES],
					     const struct tablebase*, const struct position*, double);

/* auxillary functionality */
static inline	int			 is_win(tb_entry);
static inline	int			 is_draw(tb_entry);
static inline	int			 is_loss(tb_entry);
static inline	int			 get_dtm(tb_entry);
static inline	tb_entry		 next_dtm(tb_entry);
static inline	tb_entry		 prev_dtm(tb_entry);
static inline	int			 wdl_compare(tb_entry, tb_entry);

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

/*
 * Return the tb_entry that results when playing the best move from a
 * position with value e.  If e indicates an immediate win, the result
 * is undefined.
 */
static inline tb_entry
next_dtm(tb_entry e)
{
	if (e > 0)
		return (1 - e);
	else /* e <= 0 */
		return (-e);
}

/*
 * Return the tb_entry from which we came when the best move led to this
 * position.
 */
static inline
tb_entry
prev_dtm(tb_entry e)
{
	if (e < 0)
		return (1 - e);
	else /* e >= 0 */
		return (-e);
}

/*
 * Order e and f by how good they are.  Return a negative, zero, or
 * positive value if e indicates a worse, equal, or better position than
 * f.
 */
static inline int
wdl_compare(tb_entry e, tb_entry f)
{
	if (is_loss(f))
		return (is_loss(e) ? f - e : 1);
	else if (is_draw(f))
		return (e);
	else /* is_win(f) */
		return (is_win(e) ? f - e : -1);
}

#endif /* TABLEBASE_H */
