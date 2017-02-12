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
#include <stdio.h>
#include <string.h>

#include "dobutsu.h"

/*
 * This table returns for each square number the corresponding offset in
 * a position string.
 */
static const unsigned char coordinates[SQUARE_COUNT] = {
	16, 15, 14, 12, 11, 10, 8, 7, 6, 4, 3, 2
};

/*
 * Piece names used for display, render, and memchr();
 */
static const char sente_pieces[PIECE_COUNT] = {
	[CHCK_S] = 'C',
	[CHCK_G] = 'C',
	[GIRA_S] = 'G',
	[GIRA_G] = 'G',
	[ELPH_S] = 'E',
	[ELPH_G] = 'E',
	[LION_S] = 'L',
	[LION_G] = 'L'  /* should not happen */
}, gote_pieces[PIECE_COUNT] = {
	[CHCK_S] = 'c',
	[CHCK_G] = 'c',
	[GIRA_S] = 'g',
	[GIRA_G] = 'g',
	[ELPH_S] = 'e',
	[ELPH_G] = 'e',
	[LION_S] = 'l', /* should not happen */
	[LION_G] = 'l'
}, sg_pieces[PIECE_COUNT] = {
	[CHCK_S] = 'C',
	[CHCK_G] = 'c',
	[GIRA_S] = 'G',
	[GIRA_G] = 'g',
	[ELPH_S] = 'E',
	[ELPH_G] = 'e',
	[LION_S] = 'L',
	[LION_G] = 'l',
};

/*
 * Square names used for displaying moves.
 */
static const char squares[12][2] = {
	"c4", "b4", "a4",
	"c3", "b3", "a3",
	"c2", "b2", "a2",
	"c1", "b1", "a1"
};

/*
 * Return the letter used to represent piece pc in pos.
 */
static char
piece_letter(size_t pc, const struct position *p)
{
	if (is_promoted(pc, p))
		return (p->pieces[pc] & GOTE_PIECE ? 'r' : 'R');
	else
		return (p->pieces[pc] & GOTE_PIECE ? gote_pieces[pc] : sente_pieces[pc]);
}

/*
 * Draw a nice graphical representation of a game position.  For
 * example, the position G/l-R/-e-/-C-/L--/Geg is represented as
 *
 *      ABC
 *     +---+
 *    1|l R| *eg
 *    2| e |
 *    3| C |
 *    4|L  | G
 *     +---+
 */
extern void
position_render(char render[MAX_RENDER], const struct position *p)
{
	char board[SQUARE_COUNT], sente_hand[8], gote_hand[8];
	size_t sente_count = 0, gote_count = 0, i;

	/* empty squares are spaces, no \0 terminator! */
	memset(board, ' ', sizeof board);

	if (gote_moves(p))
		gote_hand[gote_count++] = '*';
	else
		sente_hand[sente_count++] = '*';

	/* place pieces on the board */
	for (i = 0; i < PIECE_COUNT; i++) {
		if (piece_in(HAND, p->pieces[i])) {
			if (p->pieces[i] & GOTE_PIECE)
				gote_hand[gote_count++] = piece_letter(i, p);
			else
				sente_hand[sente_count++] = piece_letter(i, p);
		} else
			/* (11 - square) for coordinate transform */
			board[11 - (p->pieces[i] & ~GOTE_PIECE)] = piece_letter(i, p);
	}

	gote_hand[gote_count] = '\0';
	sente_hand[sente_count] = '\0';

	sprintf(render, "  ABC \n +---+\n1|%.3s| %s\n2|%.3s|\n3|%.3s|\n4|%.3s| %s\n +---+\n",
	    board + 0, gote_hand, board + 3, board + 6, board + 9, sente_hand);
}

/*
 * Make a string describing the position p in a syntax similar to the
 * Forsyth-Edwards-Notation used by international chess.  For example,
 * the initial position is encoded as S/gle/-c-/-C-/ELG/- with S or G
 * in the beginning encoding who makes the next move and the last part
 * indicating the pieces in hand or - if there are none.
 */
extern void
position_string(char render[MAX_POSSTR], const struct position *p)
{

	char hand[8];
	size_t i, hand_count = 0;

	render[0] = gote_moves(p) ? 'G' : 'S';
	strcpy(render + 1, "/---/---/---/---/");

	for (i = 0; i < PIECE_COUNT; i++)
		if (piece_in(HAND, p->pieces[i]))
			hand[hand_count++] = piece_letter(i, p);
		else
			render[coordinates[p->pieces[i] & ~GOTE_PIECE]] = piece_letter(i, p);

	if (hand_count == 0)
		hand[hand_count++] = '-';

	hand[hand_count] = '\0';

	strcat(render, hand);
}

/*
 * Display a move in modified algebraic notation.  The differences are:
 *  - check and mate are not indicated
 *  - a drop is indicated by an asterisk followed by the drop square
 *  - a promotion is indicated by an appended +
 */
extern void
move_string(char render[MAX_MOVSTR], const struct position *p, const struct move *m)
{
	render[0] = sente_pieces[m->piece];
	if (piece_in(HAND, p->pieces[m->piece]))
		memcpy(render + 1, "  *", 3);
	else {
		memcpy(render + 1, squares[p->pieces[m->piece] & ~GOTE_PIECE], 2);
		render[3] = piece_in(swap_colors(p->map), m->to) ? 'x' : '-';
	}

	memcpy(render + 4, squares[m->to & ~GOTE_PIECE], 2);
	if (!piece_in(HAND, p->pieces[m->piece])
	    && !is_promoted(m->piece, p)
	    && (m->piece == CHCK_S || m->piece == CHCK_G)
	    && piece_in(gote_moves(p) ? PROMZ_G : PROMZ_S, m->to))
		memcpy(render + 6, "+", 2);
	else
		render[6] = '\0';
}

/*
 * Assign square sq to the piece represented by letter pc.  If both
 * pieces of that type have already been placed or if we can't figure
 * out what piece that's supposed to be, -1 is returned.  No other
 * invariants are checked, this is done by a call to position_valid()
 * later on.
 */
static int
place_piece(char pcltr, size_t sq, struct position *p)
{
	const char *pcptr;
	size_t pc;
	unsigned needs_promotion = 0;

	pcptr = memchr(sg_pieces, pcltr, PIECE_COUNT);
	if (pcptr != NULL)
		pc = pcptr - sg_pieces;
	else {
		needs_promotion = 1;
		if (pcltr == 'R')
			pc = CHCK_S;
		else if (pcltr == 'r')
			pc = CHCK_G;
		else
			return (-1);
	}

	if (pc & 1)
		sq |= GOTE_PIECE;

	if (p->pieces[pc] == 0xff) {
		p->pieces[pc] = sq;
		p->status |= needs_promotion << pc;
		return (0);
	}

	if (p->pieces[pc ^ 1] == 0xff) {
		p->pieces[pc ^ 1] = sq;
		p->status |= needs_promotion << (pc ^ 1);
		return (0);
	}

	return (-1);
}

/*
 * Parse a position string and fill p with the information derived from
 * it.  On success, return 0, on failure, return -1.  On error, the
 * content of p is undefined.
 */
extern int
parse_position(struct position *p, const char code[MAX_POSSTR])
{
	size_t i;

	/* quick sanity check */
	if (strlen(code) < 18)
		return (-1);

	memset(p->pieces, -1, PIECE_COUNT);

	if (code[0] == 'S')
		p->status = 0;
	else if (code[0] == 'G')
		p->status = GOTE_MOVES;
	else
		return (-1);

	/* first, parse pieces in hand */
	for (i = 18; code[i] != '\0'; i++) {
		if (code[i] == '-')
			break;

		if (place_piece(code[i], IN_HAND, p))
			return (-1);
	}

	/* next, place pieces on the board */
	for (i = 0; i < SQUARE_COUNT; i++) {
		if (code[coordinates[i]] == '-')
			continue;

		if (place_piece(code[coordinates[i]], i, p))
			return (-1);
	}

	/* check if any pieces weren't assigned */
	if (memchr(p->pieces, -1, PIECE_COUNT) != NULL)
		return (-1);

	populate_map(p);

	return (position_valid(p) ? 0 : -1);
}

/*
 * Parse a square name as used in a move string.  The square name must
 * be given in lowercase.  -1 is returned if no valid square name can be
 * parsed.  index is advanced by 2 in case of success.
 */
static int
parse_square(const char *code, size_t *index)
{
	int square;

	if (code[*index] == '\0' || code[*index + 1] == '\0')
		return (-1);

	switch (code[*index]) {
	case 'a':
		square = 2;
		break;
	case 'b':
		square = 1;
		break;
	case 'c':
		square = 0;
		break;
	default:
		return (-1);
	}

	if (code[*index + 1] < '1' || code[*index + 1] > '4')
		return (-1);

	square += 3 * ('4' - code[*index + 1]);
	*index += 2;

	return (square);
}

/*
 * Parse a move string.  Spaces in the string are ignored, except if
 * they occur in the beginning.  This function returns 0 on success,
 * -1 if the string cannot be parsed into a valid move.  -1 is also
 * returned when the move would be valid but does not apply to the
 * current position.  On error, the content of m is undefined.
 */
extern int
parse_move(struct move *m, const struct position *p, const char code[MAX_MOVSTR])
{
	size_t index = 0;
	int from;
	const char *pcptr;

	pcptr = memchr(sg_pieces, code[index], PIECE_COUNT);
	if (pcptr == NULL) {
		/* promotion status is not strictly checked */
		if (code[index] == 'R' || code[index] == 'r')
			m->piece = CHCK_S;
		else
			return (-1);
	} else
		m->piece = pcptr - sg_pieces;

	while (code[++index] == ' ')
		;

	/* drop? */
	if (code[index] == '*') {
		from = IN_HAND;
		index++;
	} else {
		from = parse_square(code, &index);
		if (from == -1)
			return (-1);
	}

	if (gote_moves(p))
		from |= GOTE_PIECE;

	if (p->pieces[m->piece] != from ) {
		m->piece ^= 1;
		if (p->pieces[m->piece] != from)
			return (-1);
	}

	/* skip spaces and -/x separating squares */
	while (code[index] == ' ')
		index++;

	if (code[index] == '\0')
		return (-1);

	if (code[index] == 'x' || code[index] == '-')
		index++;

	while (code[index] == ' ')
		index++;

	/* parse destination square */
	m->to = parse_square(code, &index);
	if (m->to == 0xff)
		return (-1);

	if (gote_moves(p))
		m->to |= GOTE_PIECE;

	return (move_valid(p, m) ? 0 : -1);
}
