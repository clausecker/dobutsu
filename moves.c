#define _XSI_SOURCE 700
#include <assert.h>
#include <strings.h>

#include "dobutsu.h"

static void add_moves(unsigned, unsigned, unsigned, unsigned, unsigned, struct move*, unsigned*);
static void occupied_positions(const struct position *p, unsigned *restrict, unsigned *restrict);

/*
 * Compute all possible moves out of the current position for sente_squares.
 * This function saves the generated moves to moves which should be
 * large enough to accomodate MAX_MOVES moves.  This function returns
 * the number of moves generated.
 */
extern unsigned
generate_all_moves(struct move *moves, const struct position *p)
{
	unsigned sente_squares, gote_squares, free_squares;
	unsigned move_count = 0;

	occupied_positions(p, &sente_squares, &gote_squares);

	free_squares = ALL_SQUARES & ~(sente_squares | gote_squares);

	add_moves(PIECE_L, p->L, Llmoves[p->L], sente_squares, free_squares, moves, &move_count);
	if (p->op & Co)
		add_moves(PIECE_C, p->C, p->op & Cp ? Rmoves[p->C] : Cmoves[p->C], sente_squares, free_squares, moves, &move_count);
	/* if both chicks can be dropped, only generate moves to drop one of them */
	if (p->op & co && (p->C != IN_HAND || (p->op & Co) == 0 || p->c != IN_HAND))
		add_moves(PIECE_c, p->c, p->op & cp ? Rmoves[p->c] : Cmoves[p->c], sente_squares, free_squares, moves, &move_count);
	if (p->op & Eo)
		add_moves(PIECE_E, p->E, Eemoves[p->E], sente_squares, free_squares, moves, &move_count);
	if (p->op & eo && (p->E != IN_HAND || (p->op & Eo) == 0 || p->e != IN_HAND))
		add_moves(PIECE_e, p->e, Eemoves[p->e], sente_squares, free_squares, moves, &move_count);
	if (p->op & Go)
		add_moves(PIECE_G, p->G, Ggmoves[p->G], sente_squares, free_squares, moves, &move_count);
	if (p->op & go && (p->G != IN_HAND || (p->op & Go) == 0 || p->g != IN_HAND))
		add_moves(PIECE_g, p->g, Ggmoves[p->g], sente_squares, free_squares, moves, &move_count);

	return (move_count);
}

/*
 * This function is similar to generate_all_moves() but leaves out some
 * moves that never need to be played, these are moves that put yourself
 * into check by moving your lion adjacent to the gote lion and moves
 * that move the lion into the promotion zone (if such a move is
 * possible without putting yourself into check, the position is already
 * flagged as POS_SENTE by check_pos).
 */
extern unsigned
generate_most_moves(struct move *moves, const struct position *p)
{
	unsigned sente_squares, gote_squares, free_squares;
	unsigned move_count = 0;

	occupied_positions(p, &sente_squares, &gote_squares);

	free_squares = ALL_SQUARES & ~(sente_squares | gote_squares);

	/* don't give check with your own lion, that's stupid */
	/* also, don't try to ascend, if we could, check_pos() would have told us that we could */
	add_moves(PIECE_L, p->L, Llmoves[p->L] & ~Llmoves[p->l] & 00777, sente_squares, free_squares, moves, &move_count);
	if (p->op & Co)
		add_moves(PIECE_C, p->C, p->op & Cp ? Rmoves[p->C] : Cmoves[p->C], sente_squares, free_squares, moves, &move_count);
	if (p->op & co && (p->C != IN_HAND || (p->op & Co) == 0 || p->c != IN_HAND))
		add_moves(PIECE_c, p->c, p->op & cp ? Rmoves[p->c] : Cmoves[p->c], sente_squares, free_squares, moves, &move_count);
	if (p->op & Eo)
		add_moves(PIECE_E, p->E, Eemoves[p->E], sente_squares, free_squares, moves, &move_count);
	if (p->op & eo && (p->E != IN_HAND || (p->op & Eo) == 0 || p->e != IN_HAND))
		add_moves(PIECE_e, p->e, Eemoves[p->e], sente_squares, free_squares, moves, &move_count);
	if (p->op & Go)
		add_moves(PIECE_G, p->G, Ggmoves[p->G], sente_squares, free_squares, moves, &move_count);
	if (p->op & go && (p->G != IN_HAND || (p->op & Go) == 0 || p->g != IN_HAND))
		add_moves(PIECE_g, p->g, Ggmoves[p->g], sente_squares, free_squares, moves, &move_count);

	return (move_count);
}

/*
 * Enumerate the squares occupied by sente and gote and store them in
 * sente and gote as bitmaps.
 */
static void
occupied_positions(const struct position *p, unsigned *restrict sente, unsigned *restrict gote)
{
	unsigned sente_squares = 1 << p->L, gote_squares = 1 << p->l;

	/* first, find out which fields are occupied */
	if (p->op & Co)
		sente_squares |= 1 << p->C;
	else
		gote_squares |= 1 << p->C;

	if (p->op & co)
		sente_squares |= 1 << p->c;
	else
		gote_squares |= 1 << p->c;
	if (p->op & Eo)
		sente_squares |= 1 << p->E;
	else
		gote_squares |= 1 << p->E;
	if (p->op & eo)
		sente_squares |= 1 << p->e;
	else
		gote_squares |= 1 << p->e;
	if (p->op & Go)
		sente_squares |= 1 << p->G;
	else
		gote_squares |= 1 << p->G;
	if (p->op & go)
		sente_squares |= 1 << p->g;
	else
		gote_squares |= 1 << p->g;

	*sente = sente_squares, *gote = gote_squares;
}

/*
 * This little helper function adds the moves for piece located on the
 * square from which can go to the squares indicated by legal_moves except
 * those in blocked_squares unless it's in hand in which cases it can
 * go to all free_squares.  The moves are added to the end of moves as
 * indicated by move_count which is updated afterwards.
 */
static void
add_moves(unsigned piece, unsigned from, unsigned legal_moves,
    unsigned blocked_squares, unsigned drop_squares, struct move *moves,
    unsigned *move_count)
{
	unsigned target_squares, target;

	target_squares = from == IN_HAND ? drop_squares : legal_moves & ~blocked_squares;

	/* TODO: Can we get rid of ffs() to avoid depending on POSIX? */
	while (target_squares != 0) {
		target = ffs(target_squares) - 1;
		target_squares &= ~(1 << target);
		moves[*move_count].piece = piece;
		moves[(*move_count)++].to = target;
	}
}

/*
 * Generate all moves for the player who's turn it is (0: Gote, 1: Sente).
 */
extern unsigned
generate_all_moves_for(int turn, struct move *moves, const struct position *p)
{

	if (turn == 0) {
		unsigned move_count, i;
		struct position pos = *p;

		turn_position(&pos);
		move_count = generate_all_moves(moves, &pos);
		for (i = 0; i < move_count; i++)
			turn_move(moves + i);

		return move_count;
	} else
		return (generate_all_moves(moves, p));
}

/*
 * turn the board by 180 degrees such that what was previously sente is
 * now gote and vice versa.
 */
extern void
turn_position(struct position *p)
{
	unsigned char tmp = turned_board[p->l];

	p->l = turned_board[p->L];
	p->L = tmp;
	p->c = turned_board[p->c];
	p->C = turned_board[p->C];
	p->e = turned_board[p->e];
	p->E = turned_board[p->E];
	p->g = turned_board[p->g];
	p->G = turned_board[p->G];

	p->op ^= co | Co | eo | Eo | go | Go;
}

/*
 * Turn a struct move in the same way turn_position() turns a position.
 */
extern void
turn_move(struct move *m)
{
	if (m->piece == PIECE_L)
		m->piece = PIECE_l;
	else if (m->piece == PIECE_l)
		m->piece = PIECE_L;

	m->to = turned_board[m->to];
}

/*
 * Apply move to position, setting position to the position after the move has
 * been performed.  No sanity checks are performed.
 */
extern void
apply_move(struct position *p, const struct move m)
{

	/* first, moves pieces out of the way */
	/* assume we don't take any lion */
	if (p->c == m.to) {
		p->c = IN_HAND;
		p->op &= ~cp;
		p->op |= co;
	}

	if (p->C == m.to) {
		p->C = IN_HAND;
		p->op &= ~Cp;
		p->op |= Co;
	}

	if (p->e == m.to) {
		p->e = IN_HAND;
		p->op |= eo;
	}

	if (p->E == m.to) {
		p->E = IN_HAND;
		p->op |= Eo;
	}

	if (p->g == m.to) {
		p->g = IN_HAND;
		p->op |= go;
	}

	if (p->G == m.to) {
		p->G = IN_HAND;
		p->op |= Go;
	}

	/* if we move a chick into the promotion zone, promote it */
	if (m.to >= 9 && PIDX(p, m.piece) != IN_HAND) {
		if (m.piece == PIECE_c)
			p->op |= cp;

		if (m.piece == PIECE_C)
			p->op |= Cp;
	}

	/* not undefined behaviour! */
	PIDX(p, m.piece) = m.to;
}
