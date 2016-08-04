#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "dobutsu.h"

/*
 * Display the board described by pos to stdout.  Return
 * 0 on success, -1 on failure with errno set to a reason.
 * This function does not make sure that p is actually
 * valid and crashes horribly if it isn't.
 */
extern int
display_pos(const struct position *p)
{
	/* the 13th byte is never read but still useful */
	char board[13];
	char sente_hand[7] = "", gote_hand[7] = "";
	int sente_count = 0, gote_count = 0;

	memset(board, ' ', 12);

	/* place lions */
	board[11 - p->L] = 'L';
	board[11 - p->l] = 'l';

	/* minor pieces */
	if (p->op & Go)
		if (p->G > 11)
			sente_hand[sente_count++] = 'G';
		else
			board[11 - p->G] = 'G';
	else
		if (p->G > 11)
			gote_hand[gote_count++] = 'g';
		else
			board[11 - p->G] = 'g';

	if (p->op & go)
		if (p->g > 11)
			sente_hand[sente_count++] = 'G';
		else
			board[11 - p->g] = 'G';
	else
		if (p->g > 11)
			gote_hand[gote_count++] = 'g';
		else
			board[11 - p->g] = 'g';

	if (p->op & Eo)
		if (p->E > 11)
			sente_hand[sente_count++] = 'E';
		else
			board[11 - p->E] = 'E';
	else
		if (p->E > 11)
			gote_hand[gote_count++] = 'e';
		else
			board[11 - p->E] = 'e';

	if (p->op & eo)
		if (p->e > 11)
			sente_hand[sente_count++] = 'E';
		else
			board[11 - p->e] = 'E';
	else
		if (p->e > 11)
			gote_hand[gote_count++] = 'e';
		else
			board[11 - p->e] = 'e';

	if (p->op & Co)
		if (p->C > 11)
			sente_hand[sente_count++] = 'C';
		else
			board[11 - p->C] = p->op & Cp ? 'R' : 'C';
	else
		if (p->C > 11)
			gote_hand[gote_count++] = 'c';
		else
			board[11 - p->C] = p->op & Cp ? 'r' : 'c';

	if (p->op & co)
		if (p->c > 11)
			sente_hand[sente_count++] = 'C';
		else
			board[11 - p->c] = p->op & cp ? 'R' : 'C';
	else
		if (p->c > 11)
			gote_hand[gote_count++] = 'c';
		else
			board[11 - p->c] = p->op & cp ? 'r' : 'c';

	return (printf("+---+\n" "|%.3s| %s\n" "|%.3s|\n" "|%.3s|\n" "|%.3s| %s\n" "+---+\n",
	    board + 0, gote_hand, board + 3, board + 6, board + 9, sente_hand) >= 0 ? 0 : -1);
}

/* display the content of a position struct */
extern int
show_pos(const struct position *p)
{
	return (printf("(%2d,%2d,%2d,%2d,%2d,%2d,%2d,%2d,%02x)",
	    p->c, p->C, p->g, p->G, p->e, p->E, p->l, p->L, p->op) >= 0 ? 0 : -1);
}

static const char *const long_pieces[MAX_PIECE + 1] = {
	[PIECE_l] = "lion",
	[PIECE_L] = "lion",
	[PIECE_c] = "chick",
	[PIECE_C] = "chick",
	[PIECE_e] = "elephant",
	[PIECE_E] = "elephant",
	[PIECE_g] = "giraffe",
	[PIECE_G] = "giraffe"
};

static const char short_pieces[MAX_PIECE + 1] = {
	[PIECE_l] = 'L',
	[PIECE_L] = 'L',
	[PIECE_c] = 'C',
	[PIECE_C] = 'C',
	[PIECE_e] = 'E',
	[PIECE_E] = 'E',
	[PIECE_g] = 'G',
	[PIECE_G] = 'G'
};

static const char places[13] = "0123456789AB*";

/* print a human-readable description of a move */
extern int display_move(const struct position *p, struct move m)
{
	const char *piece = long_pieces[m.piece], *promote = "";

	if (m.piece == PIECE_c || m.piece == PIECE_C) {
		if (m.piece == PIECE_c && p->op & cp || m.piece == PIECE_C && p->op & Cp)
			piece = "rooster";
		else if (m.to > 8)
			promote = " promote";
	}

	if (PIDX(p, m.piece) == IN_HAND)
		return (printf("drop %s to %c\n", piece, places[m.to]));
	else
		return (printf("%c %s to %c%s\n", places[PIDX(p, m.piece)], piece, places[m.to], promote));
}

/* print a move in algebraic notation */
extern int show_move(const struct position *p, struct move m)
{
	char piece = short_pieces[m.piece], promote = ' ', printout[5];

	if (m.piece == PIECE_c || m.piece == PIECE_C) {
		if (m.piece == PIECE_c && p->op & cp || m.piece == PIECE_C && p->op & Cp)
			piece = 'R';
		else if (m.to > 8 && PIDX(p, m.piece) != IN_HAND)
			promote = ' ';
	}

	printout[0] = places[PIDX(p, m.piece)];
	printout[1] = piece;
	printout[2] = places[m.to];
	printout[3] = promote;
	printout[4] = '\0';

	return (fputs(printout, stdout));
}
