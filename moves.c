#define _XSI_SOURCE 700
#include <assert.h>
#include <stddef.h>
#include <strings.h>
#include <stdio.h>

#include "dobutsu.h"

static void add_moves(unsigned, unsigned, const board*, unsigned, unsigned, struct move*, unsigned*);

/*
 * Compute all possible moves out of the current position for sente_squares.
 * This function saves the generated moves to moves which should be
 * large enough to accomodate MAX_MOVES moves.  This function returns
 * the number of moves generated.
 */
extern unsigned
generate_moves(struct move *moves, const struct position *p)
{
	unsigned sente_squares = 1 << p->L, gote_squares = 1 << p->l, free_squares;
	unsigned move_count = 0;

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

	free_squares = ALL_SQUARES & ~(sente_squares | gote_squares);

	add_moves(PIECE_L, p->L, Lmoves, sente_squares, free_squares, moves, &move_count);
	if (p->op & Co)
		add_moves(PIECE_C, p->C, p->op & Cp ? Rmoves : Cmoves, sente_squares, free_squares, moves, &move_count);
	if (p->op & co)
		add_moves(PIECE_c, p->c, p->op & cp ? Rmoves : Cmoves, sente_squares, free_squares, moves, &move_count);
	if (p->op & Eo)
		add_moves(PIECE_E, p->E, Eemoves, sente_squares, free_squares, moves, &move_count);
	if (p->op & eo)
		add_moves(PIECE_e, p->e, Eemoves, sente_squares, free_squares, moves, &move_count);
	if (p->op & Go)
		add_moves(PIECE_G, p->G, Ggmoves, sente_squares, free_squares, moves, &move_count);
	if (p->op & go)
		add_moves(PIECE_g, p->g, Ggmoves, sente_squares, free_squares, moves, &move_count);

	return (move_count);
}

/*
 * This little helper function adds the moves for piece located on the
 * square from which can go to the squares indicated by movetab except
 * those in blocked_squares unless it's in hand in which cases it can
 * go to all free_squares.  The moves are added to the end of moves as
 * indicated by move_count which is updated afterwards.
 */
static void
add_moves(unsigned piece, unsigned from, const board *movetab,
    unsigned blocked_squares, unsigned drop_squares, struct move *moves,
    unsigned *move_count)
{
	unsigned target_squares, target;

	target_squares = from == IN_HAND ? drop_squares : movetab[from] & ~blocked_squares;

	/* TODO: Can we get rid of ffs() to avoid depending on POSIX? */
	while (target_squares != 0) {
		target = ffs(target_squares) - 1;
		target_squares &= ~(1 << target);
		moves[*move_count].piece = piece;
		moves[(*move_count)++].to = target;
	}
}

/*
 * turn the board by 180 degrees such that what was previously sente is
 * now gote and vice versa.
 */
extern void
turn_position(struct position *p)
{
	unsigned char tmp = turn_board[p->l];

	p->l = turn_board[p->L];
	p->L = tmp;
	p->c = turn_board[p->c];
	p->C = turn_board[p->C];
	p->e = turn_board[p->e];
	p->E = turn_board[p->E];
	p->g = turn_board[p->g];
	p->G = turn_board[p->G];

	p->op ^= co | Co | eo | Eo | go | Go;
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
