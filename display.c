#include <stdio.h>
#include <string.h>

#include "dobutsu.h"

/*
 * Piece names used for display and render.
 */
static const char sente_pieces[PIECE_COUNT] = {
	[LION_S] = 'L',
	[LION_G] = 'L', /* should not happen */
	[GIRA_S] = 'G',
	[GIRA_G] = 'G',
	[ELPH_S] = 'E',
	[ELPH_G] = 'E',
	[CHCK_S] = 'C',
	[CHCK_G] = 'C'
}, gote_pieces[PIECE_COUNT] = {
	[LION_S] = 'l', /* should not happen */
	[LION_G] = 'l',
	[GIRA_S] = 'g',
	[GIRA_G] = 'g',
	[ELPH_S] = 'e',
	[ELPH_G] = 'e',
	[CHCK_S] = 'c',
	[CHCK_G] = 'c'
};

/*
 * Return the letter used to represent piece pc in pos.
 */
static char
piece_letter(size_t pc, const struct position *p)
{
	if (p->status & 1 << pc)
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
			/* (12 - square) for coordinate transform */
			board[12 - (p->pieces[i] & ~GOTE_PIECE)] = piece_letter(i, p);
	}

	gote_hand[gote_count] = '\0';
	sente_hand[sente_count] = '\0';

	sprintf(render, " ABC \n +---+\n1|%.3s| %s\n2|%.3s|\n3|%.3s|\n4|%.3s| %s\n +---+\n",
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
	/*
	 * Transform the internal square numbers into indices into the
	 * output.
	 */
	static const unsigned char coordinates[SQUARE_COUNT] = {
		16, 15, 14, 12, 11, 10, 8, 7, 6, 4, 3, 2
	};

	char hand[8];
	size_t i, hand_count = 0;

	render[0] = gote_moves(p) ? 'G' : 'S';
	strcpy(render + 1, "/---/---/---/---/");

	for (i = 0; i < PIECE_COUNT; i++)
		if (piece_in(HAND, p->pieces[i]))
			hand[hand_count++] = piece_letter(i, p);
		else
			render[coordinates[i]] = piece_letter(i, p);

	if (hand_count == 0)
		hand[hand_count++] = '-';

	hand[hand_count] = '\0';

	strcat(render, hand);
}
