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

/*
 * This is the non-generic interface to the position and move
 * structures.  It contains some convenience functions to effectively
 * analyse and modify these structures.
 */

#include <stddef.h>
#include <stdint.h>

#include "rules.h"

/* various bitmaps to build boards */
enum {
        BOARD_S = 007777,
	PROMZ_S = 007000,
	PROMZ_G = 000007 << GOTE_PIECE,
        BOARD_G = BOARD_S << GOTE_PIECE,
        BOARD   = BOARD_S | BOARD_G,
        HAND_S  = 010000,
        HAND_G  = HAND_S << GOTE_PIECE,
        HAND    = HAND_S | HAND_G,
        VALID_SQUARE = BOARD | HAND,
        MAX_SQUARE = GOTE_PIECE + SQUARE_COUNT,
};

static inline	void	populate_map(struct position*);
static inline	int	square_valid(unsigned);
static inline	int	piece_in(board, unsigned);
static inline	int	piece_in_nosg(board, unsigned);
static inline	int	gote_owns(unsigned);
static inline	int	is_promoted(unsigned, const struct position*);
static inline	board	swap_colors(board);
extern		board	moves_for(unsigned, const struct position*);
extern const	board	roostertab[32];

/* inline implementations */

/*
 * Fill p->map with the piece positions.
 */
static inline
void populate_map(struct position *p)
{
	size_t i;

	p->map = 0;

	for (i = 0; i < PIECE_COUNT; i++)
		p->map |= 1 << p->pieces[i];

	p->map &= BOARD;
}

/*
 * Return 1 if sq is a values value for a
 * struct position.pieces[] member.
 */
static inline int
square_valid(unsigned sq)
{

	return !!((sq & ~GOTE_PIECE) <= SQUARE_COUNT);
}

/*
 * Return 1 if the bitmap indicates 1 for sq.
 */
static inline int
piece_in(board b, unsigned sq)
{

	return !!(b & 1 << sq);
}

/*
 * Return 1 if the bitmap indicates 1 for sq, not
 * accounting for the difference between sente and gote.
 */
static inline int
piece_in_nosg(board b, unsigned sq)
{
	return !!((b | b >> GOTE_PIECE) & 1 << (sq & ~GOTE_PIECE));
}

/*
 * Return 1 if gote owns piece pc.
 */
static inline int
gote_owns(unsigned pc)
{

	return !!(pc & GOTE_PIECE);
}

/*
 * Return 1 if pc is promoted.
 */
static inline int
is_promoted(unsigned pc, const struct position *p)
{

	return !!(p->status & 1 << pc);
}

/*
 * swap the colours in a board.
 */
static inline
board swap_colors(board b)
{
	return (b << GOTE_PIECE | b >> GOTE_PIECE);
}
