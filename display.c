#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "dobutsu.h"

static void fill_board(const struct position*, char*, char*, char*);

/*
 * Auxillary function for display_pos() and pos_notation(): Fill in board
 * with what pieces occupy each square, sente_hand with the pieces in
 * sente's hand and gote_hand with the pieces in gote's hand.
 */
static void
fill_board(const struct position *p, char *board, char *sente_hand, char *gote_hand)
{
	int sente_count = 0, gote_count = 0;	

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

	sente_hand[sente_count] = '\0';
	gote_hand[gote_count] = '\0';
}

/*
 * Display the board described by pos to stdout.  Return
 * 0 on success, -1 on failure with errno set to a reason.
 * This function does not make sure that p is actually
 * valid and crashes horribly if it isn't.
 */
extern int
display_pos(const struct position *p)
{
	char board[12], sente_hand[7], gote_hand[7];

	memset(board, ' ', sizeof board);
	fill_board(p, board, sente_hand, gote_hand);

	return (printf("+---+\n" "|%.3s| %s\n" "|%.3s|\n" "|%.3s|\n" "|%.3s| %s\n" "+---+\n",
	    board + 0, gote_hand, board + 3, board + 6, board + 9, sente_hand) >= 0 ? 0 : -1);
}

/*
 * generate a position notation string for p with turn indicating whose
 * move it is (0 for Gote, 1 for Sente).
 */
extern void
pos_notation(char *out, int turn, const struct position *p)
{
	char board[12], sente_hand[7], gote_hand[7];

	memset(board, '-', sizeof board);
	fill_board(p, board, sente_hand, gote_hand);
	if (turn == 0)
		out[0] = 'G';
	else
		out[0] = 'S';

	out[1] = '/';
	memcpy(out +  2, board + 0, 3);
	out[5] = '/';
	memcpy(out +  6, board + 3, 3);
	out[9] = '/';
	memcpy(out + 10, board + 6, 3);
	out[13] = '/';
	memcpy(out + 14, board + 9, 3);
	out[17] = '/';
	if (sente_hand[0] == '\0' && gote_hand[0] == '\0') {
		out[18] = '-';
		out[19] = '\0';
	} else {
		strcpy(out + 18, sente_hand);
		strcat(out + 18, gote_hand);
	}
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

static const char short_pieces_sente[MAX_PIECE + 1] = {
	[PIECE_l] = 'L',
	[PIECE_L] = 'L',
	[PIECE_c] = 'C',
	[PIECE_C] = 'C',
	[PIECE_e] = 'E',
	[PIECE_E] = 'E',
	[PIECE_g] = 'G',
	[PIECE_G] = 'G'
};

static const char short_pieces_gote[MAX_PIECE + 1] = {
	[PIECE_l] = 'l',
	[PIECE_L] = 'l',
	[PIECE_c] = 'c',
	[PIECE_C] = 'c',
	[PIECE_e] = 'e',
	[PIECE_E] = 'e',
	[PIECE_g] = 'g',
	[PIECE_G] = 'g'
};

static const char places[13] = "0123456789AB*";

/* print a human-readable description of a move */
extern int describe_move(const struct position *p, struct move m)
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

/* create algebraic notation for move with to_move to move */
extern void move_notation(char *printout, const struct position *p, struct move m, int to_move)
{
	char piece, promote = ' ';

	if (to_move == TURN_GOTE)
		piece = short_pieces_gote[m.piece];
	else
		piece = short_pieces_sente[m.piece];

	if (m.piece == PIECE_c || m.piece == PIECE_C) {
		if (m.piece == PIECE_c && p->op & cp || m.piece == PIECE_C && p->op & Cp)
			piece = to_move == TURN_GOTE ? 'r' : 'R';
		else if (m.to > 8 && PIDX(p, m.piece) != IN_HAND)
			promote = '+';
	}

	printout[0] = places[PIDX(p, m.piece)];
	printout[1] = piece;
	printout[2] = places[m.to];
	printout[3] = promote;
	printout[4] = '\0';
}
