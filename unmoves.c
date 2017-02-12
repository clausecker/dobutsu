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
#define _XOPEN_SOURCE 700
#include <assert.h>
#include <strings.h> /* ffs() */

#include "dobutsu.h"



/*
 * This table contains the same information as movetab but for undoing
 * moves. It roughly holds that
 *
 *     swap_colors(unmovetab[i][j]) = movetab[i][j ^ GOTE_PIECE]
 *
 * except for the following differences:
 *
 *  - a chick in the promotion zone must have been dropped there and
 *    thus can't unmove anywhere else.
 *  - a lion never came from its own promotion zone.
 */
#define MOVETAB_ENTRY(X0, X1, X2, X3, X4, X5, X6, X7, X8, X9, X10, X11) \
	{ X0, X1, X2, X3, X4, X5, X6, X7, \
	X8, X9, X10, X11, 0, 0, 0, 0, \
	X0 << GOTE_PIECE, X1 << GOTE_PIECE, X2 << GOTE_PIECE, X3 << GOTE_PIECE, \
	X4 << GOTE_PIECE, X5 << GOTE_PIECE, X6 << GOTE_PIECE, X7 << GOTE_PIECE, \
	X8 << GOTE_PIECE, X9 << GOTE_PIECE, X10 << GOTE_PIECE, X11 << GOTE_PIECE, \
	0, 0, 0, 0 }

static const board unmovetab[PIECE_COUNT/2][32] = {
	[CHCK_S/2] = {
	    00000, 00000, 00000, 00001, 00002, 00004, 00010, 00020,
	    00040, 00000, 00000, 00000, 0, 0, 0, 0,
	    00000 << GOTE_PIECE, 00000 << GOTE_PIECE, 00000 << GOTE_PIECE, 00100 << GOTE_PIECE,
	    00200 << GOTE_PIECE, 00400 << GOTE_PIECE, 01000 << GOTE_PIECE, 02000 << GOTE_PIECE,
	    04000 << GOTE_PIECE, 00000 << GOTE_PIECE, 00000 << GOTE_PIECE, 00000 << GOTE_PIECE,
	    0, 0, 0, 0
	},
	[GIRA_S/2] = MOVETAB_ENTRY(00012, 00025, 00042, 00121, 00252, 00424,
	    01210, 02520, 04240, 02100, 05200, 02400),
	[ELPH_S/2] = MOVETAB_ENTRY(00020, 00050, 00020, 00202, 00505, 00202,
	    02020, 05050, 02020, 00200, 00500, 00200),
	[LION_S/2] = {
	    00032, 00075, 00062, 00323, 00757, 00626, 00230, 00570,
	    00260, 00300, 00700, 00600, 0, 0, 0, 0,
	    00030 << GOTE_PIECE, 00070 << GOTE_PIECE, 00060 << GOTE_PIECE, 00230 << GOTE_PIECE,
	    00750 << GOTE_PIECE, 00620 << GOTE_PIECE, 03230 << GOTE_PIECE, 07570 << GOTE_PIECE,
	    06260 << GOTE_PIECE, 02300 << GOTE_PIECE, 05700 << GOTE_PIECE, 02600 << GOTE_PIECE,
	    0, 0, 0, 0
	}
};

/*
 * Compute all square from which piece pc in p could have moved in the
 * previous move.  This does not account for promotion or drops.
 */
static board
unmoves_for(unsigned pc, const struct position *p)
{
	board dst;

	/*
	 * roosters and chicks can move to squares they cannot move back
	 * from, making movetab unsuitable for looking up moves
	 * directly.  However, if we look up moves for the opposite
	 * colour and then swap colours, we get the right patterns.
	 */
	dst = is_promoted(pc, p)
	    ? swap_colors(roostertab[p->pieces[pc] ^ GOTE_PIECE])
	    : unmovetab[pc / 2][p->pieces[pc]];

	/* remove invalid source squares */
	dst &= ~p->map & ~swap_colors(p->map);

	return (dst);
}

/*
 * Generate unmove structures for piece pc that might have captured any
 * the pieces listed in uncap of length ucc while moving.
 */
static struct unmove *
generate_unmoves_for_piece(struct unmove *unmoves, const struct position *p,
    size_t pc, const size_t *uncap, size_t ucc)
{
	struct unmove um;
	int gote_moved = !gote_moves(p);
	size_t i, j, i0 = gote_moved * GOTE_PIECE;
	board src_squares = unmoves_for(pc, p);

	um.piece = pc;

	/* iterate over all occupied squares in src_squares */
	while (src_squares != 0) {
		i = ffs(src_squares) - 1;

		um.from = i;
		um.status = 0;
		um.capture = -1;
		*unmoves++ = um;

		/* account for capture */
		for (j = 0; j < ucc; j++) {
			um.status = 0;
			um.capture = uncap[j];
			*unmoves++ = um;

			/* account for rooster capture */
			if (uncap[j] == CHCK_S || uncap[j] == CHCK_G) {
				um.status = 1 << uncap[j];
				*unmoves++ = um;
			}
		}

		src_squares &= ~(1 << i);
	}

	/* account for drop */
	if (pc != LION_S && pc != LION_G && !is_promoted(pc, p)) {
		um.from = i0 + IN_HAND;
		um.status = 0,
		um.capture = -1;
		*unmoves++ = um;
	}

	/* account for chicken promoting to rooster */
	if (is_promoted(pc, p)
	    && piece_in(gote_moved ? PROMZ_G : PROMZ_S, p->pieces[pc])
	    && !piece_in_nosg(p->map, p->pieces[pc] + (gote_moved ? 3 : -3))) {
		um.from = p->pieces[pc] + (gote_moved ? 3 : -3);
		um.status = 1 << pc;
		um.capture = -1;
		*unmoves++ = um;

		/* account for capture */
		for (j = 0; j < ucc; j++) {
			um.status = 1 << pc;
			um.capture = uncap[j];
			*unmoves++ = um;

			/* account for rooster capture */
			if (uncap[j] == CHCK_S || uncap[j] == CHCK_G) {
				um.status |= 1 << uncap[j];
				*unmoves++ = um;
			}
		}
	}

	return (unmoves);
}

/*
 * Generate unmove structures for all moves that lead to this position.
 */
extern size_t
generate_unmoves(struct unmove unmoves[MAX_UNMOVES], const struct position *p)
{
	struct unmove *oum = unmoves;
	size_t umc, i, uncap[PIECE_COUNT], ucc = 0;
	int gote_moved = !gote_moves(p);

	/*
	 * Check which pieces we can uncapture.  If both pieces of a
	 * kind could be uncaptured, only uncapture one of them.
	 */
	for (i = 0; i < PIECE_COUNT; i += 2) {
		if (p->pieces[i] == gote_moved * GOTE_PIECE + IN_HAND)
			uncap[ucc++] = i;
		else if (p->pieces[i + 1] == gote_moved * GOTE_PIECE + IN_HAND)
			uncap[ucc++] = i + 1;
	}

	for (i = 0; i < PIECE_COUNT; i++)
		if (gote_moved == gote_owns(p->pieces[i]) && piece_in(BOARD, p->pieces[i]))
			unmoves = generate_unmoves_for_piece(unmoves, p, i, uncap, ucc);

	umc = (size_t)(unmoves - oum);
	assert (umc <= MAX_UNMOVES);
	return (umc);
}

/*
 * Undo a move described by a struct umove.
 */
extern void
undo_move(struct position *p, const struct unmove *u)
{
	p->map &= ~(1 << p->pieces[u->piece]);
	p->map |= 1 << u->from;

	if (u->capture >= 0) {
		p->pieces[u->capture] = p->pieces[u->piece] ^ GOTE_PIECE;
		p->map |= 1 << p->pieces[u->capture];
	}

	p->map &= BOARD;
	p->pieces[u->piece] = u->from;

	p->status ^= u->status | GOTE_MOVES;
}
