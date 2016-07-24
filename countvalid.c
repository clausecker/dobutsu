#include <stdio.h>

#include "dobutsu.h"

/* count the number of valid positions */
extern int
main()
{
	pos_code i, invalid = 0, sente = 0, other, max_move_pc = -1;
	unsigned move_count, max_move_count = 0;
	struct position p;
	struct move moves[MAX_MOVES];

	for (i = 0; i < MAX_POS; i++) {
		if (i % 10000000 == 0)
			fprintf(stderr, "\r%10u", i);

		switch (decode_pos(&p, i)) {
		case POS_INVALID:
			invalid++;
			break;

		case POS_SENTE:
			sente++;
			break;

		default:
			move_count = generate_moves(moves, &p);
			if (move_count > max_move_count) {
				max_move_count = move_count;
				max_move_pc = i;
			}
		}
	}

	fprintf(stderr, "\r%10u\n\n", i);

	fprintf(stderr, "invalid: %10u (%5.2f%%)\n", invalid, (100.0*invalid)/i);
	fprintf(stderr, "won:     %10u (%5.2f%%)\n", sente,   (100.0*sente  )/i);
	other = i - invalid - sente;
	fprintf(stderr, "other:   %10u (%5.2f%%)\n", other,   (100.0*other  )/i);
	fprintf(stderr, "\nHighest amount of moves: %u at %u\n", max_move_count, max_move_pc);

	return (0);
}
