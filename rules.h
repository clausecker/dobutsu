#ifndef RULES_H
#define RULES_H

/*
 * This header contains the data structures for Doubutsu Shougi position
 * as well as functions to manipulate such a position.  The interface is
 * designed to be somewhat generic in an attempt to reduce the number of
 * changes needed to implement other Shougi variants.  Neverthless it is
 * also designed to perform well, speeding up the table base building as
 * much as possible without sacrificing performance.
 */

#include <stddef.h>

enum {
/*
 * There are eight pieces in Doubutsu Shougi, two of each: lions,
 * giraffes, elephants, and chicks.  Only ownership of the
 * lions is fixed (to Sente and Gote), the other pieces can change
 * owner.  The _S and _G suffixes are to be understood as a mean of
 * distinguishing the two pieces of the same kind in code.
 */
	LION_S, LION_G,
	GIRA_S, GIRA_G,
	ELPH_S, ELPH_G,
	CHCK_S, CHCK_G,

/* see below for explanation of these constants */
	PIECE_COUNT = 8,
	SQUARE_COUNT = 12,
	IN_HAND = 12,
	GOTE_PIECE = 16,

/* the maximal amount of moves that can exist in any given position */
	MAX_MOVES = 38,

/* various buffer lengths */
	MAX_RENDER = 100, /* guess */
	MAX_POSSTR = 25,  /* e.g. S/L--/--l/---/---/ccggee */
	MAX_MOVSTR = 8,   /* e.g. Cb2xb3+ */
	MAX_MOVDSC = 100, /* guess */	

/*
 * Status bits for struct position.  For the promotion status, 0
 * indicates an unpromoted chick, 1 indicates a promoted chick (i.e. a
 * rooster).  For move status, 0 indicates Sente to move, 1 indicates
 * Gote to move.  The indices of these bits have been carefully chosen
 * to enable some optimizations.
 */
	GOTE_MOVES = 1 <<  GOTE_PIECE, /* is it Gote's move? */
	ROST_S =     1 <<  CHCK_S,     /* is CHCK_S a rooster? */
	ROST_G =     1 <<  CHCK_G,     /* is CHCK_G a rooster? */
};

/*
 * A bitmap representing the current status of the board.  This type
 * is not part of the abstract interface but it is needed for the
 * position type.
 */
typedef unsigned board;

/*
 * A position is described as a vector of where pieces are and some
 * status bits.  Each piece position is a number between 0 and
 * SQUARE_COUNT, with SQUARE_COUNT (== IN_HAND) indicating that the
 * piece is not on the board.  To this, GOTE_PIECE (a power of two) is
 * added if the piece is owned by Gote.  This makes enumerating occupied
 * squares separated by Sente and Gote occupying them easy.  The status
 * bits are explained in the enumeration above.
 */
struct position {
	unsigned char pieces[PIECE_COUNT];
	unsigned status;
	unsigned map;
};

/*
 * A move is described by a piece and the square it moves from/to.  The
 * constant GOTE_PIECE must be added to both squares if the move is
 * performed by Gote.
 */
struct move {
	unsigned piece, to;
};

/*
 * the following functions perform common operations on positions and
 * moves.  Those functions that update a position do so in-place.  Make
 * a copy beforehand if you need the old position.  See the
 * implementations of these functions for documentation.
 */

/* validation */
extern		int	position_valid(const struct position*);
extern		int	move_valid(const struct position*, struct move);

/* status information */
static inline	int	gote_moves(const struct position*);
extern		int	sente_in_check(const struct position*);
extern		int	gote_in_check(const struct position*);

/* board modification */
extern		void	turn_board(struct position*);
extern		int	play_move(struct position*, struct move);
static inline	void	null_move(struct position*);

/* move generation */
extern		size_t	generate_moves(struct move[MAX_MOVES], const struct position*);

/* display */
extern		void	position_render(char[MAX_RENDER], const struct position*);
extern		void	position_string(char[MAX_POSSTR], const struct position*);
extern		void	move_description(char[MAX_MOVDSC], struct move);
extern		void	move_string(char[MAX_MOVSTR], struct move);

/* parsing, returns 0 on success, -1 on failure */
extern		int	parse_position(struct position*, const char[MAX_POSSTR]);
extern		int	parse_move(struct move*, const struct position*, const char[MAX_MOVSTR]);

/*
 * Some of these functions are implemented inline for performance.
 * Their implementations can be found below.
 */

/*
 * Return 1 if gote moves in p, 0 otherwise return 1.
 */
static inline
int gote_moves(const struct position *p)
{

	return !!(p->status & GOTE_MOVES);
}

/*
 * Play a null move, that is, flip the bit indicating whose turn it is.
 */
static inline
void null_move(struct position *p)
{

	p->status ^= GOTE_MOVES;
}

#endif /* RULES_H */
