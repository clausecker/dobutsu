/*-
 * Copyright (c) 2016--2017 Robert Clausecker. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include "dobutsu.h"

/*
 * Check if p describes a valid position.  The following invariants are
 * checked:
 *
 *  - check all fields for invalid values
 *  - no two pieces may occupy the same square
 *  - LION_S must be owned by Sente, LION_G by Gote
 *  - LION_S and LION_G must be on the board
 *  - LION_S cannot occupy the fourth rank, LION_G not the first
 *  - a chick in hand cannot be promoted
 *  - the board map must correspond to the pieces
 */
extern int
position_valid(const struct position *p)
{
	size_t i;
	board b;

	if (p->status & ~POS_FLAGS)
		return (0);

	for (i = 0; i < PIECE_COUNT; i++)
		if (!square_valid(p->pieces[i]))
			return (0);

	for (i = b = 0; i < PIECE_COUNT; i++) {
		if (piece_in_nosg(b, p->pieces[i]))
			return (0);

		/* ignore pieces in hand in overlap check */
		b |= 1 << p->pieces[i] & ~HAND;
	}

	if (b != p->map) {
		return (0);
	}

	if (!piece_in(BOARD_S & ~PROMZ_S, p->pieces[LION_S])
	    || !piece_in(BOARD_G & ~ PROMZ_G, p->pieces[LION_G]))
		return (0);

	if (piece_in(HAND, p->pieces[CHCK_S]) && is_promoted(CHCK_S, p)
	    || piece_in(HAND, p->pieces[CHCK_G]) && is_promoted(CHCK_G, p))
		return (0);

	return (1);
}

/*
 * Check if m describes a valid move, assuming that p describes a valid
 * position.  The following invariants are checked:
 *
 *  - the piece number must be between 0 and PIECE_COUNT - 1
 *  - the player to play must own the piece in question
 *  - the destination square must have a valid number
 *  - the GOTE_PIECE bit of the destination square must agree to
      the bit indicating who moves
 *  - if the piece is dropped, the destination square must be empty,
 *    else the destination square must be reachable and not occupied by
 *    our own pieces
 */
extern int
move_valid(const struct position *p, const struct move *m)
{

	if (m->piece >= PIECE_COUNT)
		return (0);

	if (gote_moves(p) ^ gote_owns(p->pieces[m->piece]))
		return (0);

	if (gote_moves(p) ^ gote_owns(m->to))
		return (0);

	if (!square_valid(m->to) || piece_in(HAND, m->to))
		return (0);

	if (!piece_in(moves_for(m->piece, p), m->to))
		return (0);

	if (piece_in(HAND, p->pieces[m->piece])) {
		if (piece_in_nosg(p->map, m->to))
			return (0);
	} else {
		if (piece_in(p->map, m->to))
			return (0);
	}

	return (1);
}
