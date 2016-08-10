#include <ctype.h>
#include <string.h>

#include "dobutsu.h"

/*
 * Parse a position string into p.  Return -1 if the position is
 * invalid, 1 if Sente is the one to move in the position described by
 * the string and 0 if Gote is the one to move.
 *
 * A position string has the form
 *     T/BA9/876/543/210/H
 *
 * where T is either S or G depending on whose move it is, 0-B are the
 * contents of the respective squares (one of LlCcRrGgEe) or - if there
 * is nothing on that square. Upper case indicates that the piece is
 * owned by Sente, lower case that it is owned by Gote.  H is the set of
 * pieces in hand.  See README for details.
 */
extern int
parse_position(struct position *p, const char *nstr)
{
	int turn, len, blen, i;
	char board[23];
	const char *idx;
	unsigned char *pieces;
	static const char coordinate_transform[22] = {
		0xb, 0xa, 0x9, -1, 0x8, 0x7, 0x6, -1, 0x5, 0x4, 0x3, -1, 0x2, 0x1, 0x0, -1,
		IN_HAND, IN_HAND, IN_HAND, IN_HAND, IN_HAND, IN_HAND,
	};

	/* basic syntax checks */
	len = strlen(nstr);
	if (len < 18 || len > 24) 
		return (-1);

	if (nstr[0] == 'G')
		turn = 0;
	else if (nstr[0] == 'S')
		turn = 1;
	else
		return (-1);

	len -= 2;
	memcpy(board, nstr + 2, len + 1); /* also copy trailing NUL */

	/* remove slashes */
	board[3] = '-';
	board[7] = '-';
	board[11] = '-';
	board[15] = '-';

	/* check if anything weird remains */
	if (strspn(board, "-LlCcRrGgEe") != len)
		return (-1);

	/* find lions */
	p->L = strcspn(board, "L");
	p->l = strcspn(board, "l");

	/* find chicks */
	p->op = 0;
	p->C = strcspn(board, "CcRr");
	if (isupper(board[p->C]))
		p->op |= Co;
	if (board[p->C] == 'r' || board[p->C] == 'R')
		p->op |= Cp;
	board[p->C] = '-';

	p->c = strcspn(board, "CcRr");
	if (isupper(board[p->C]))
		p->op |= co;
	if (board[p->c] == 'r' || board[p->c] == 'R')
		p->op |= Cp;

	/* find elephants */
	p->E = strcspn(board, "Ee");
	if (isupper(board[p->E]))
		p->op |= Eo;
	board[p->E] = '-';

	p->e = strcspn(board, "Ee");
	if (isupper(board[p->e]))
		p->op |= eo;

	/* find giraffes */
	p->G = strcspn(board, "Gg");
	if (isupper(board[p->G]))
		p->op |= Go;
	board[p->G] = '-';

	p->g = strcspn(board, "Gg");
	if (isupper(board[p->g]))
		p->op |= go;

	/* convert coordinates to what the program expects */
	pieces = (unsigned char*)p;
	for (i = 0; i < 8; i++) {
		if (pieces[i] == len)
			return (-1);

		pieces[i] = coordinate_transform[pieces[i]];
	}

	/* make sure that rooster is not in hand */
	if (p->c == IN_HAND && p->op & cp || p->C == IN_HAND && p->op & Cp)
		return (-1);

	/* make sure that the lions are on the board */
	if (p->L == IN_HAND || p->l == IN_HAND)
		return (-1);

	return (turn);
}
