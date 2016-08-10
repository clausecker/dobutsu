#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "dobutsu.h"

/* program to test display.c */
extern int main(int argc, char *argv[])
{
	struct position pos;
	struct move moves[MAX_MOVES];
	unsigned long parsed_pc;
	int move_count, i, turn = 0;
	char *endptr;

	pos_code pc;

	switch (argc) {
	case 1:
		srand(time(NULL));
		for (;;) {
			pc = rand();
			if (pc >= MAX_POS)
				continue;

			decode_pos(&pos, pc);
			if (check_pos(&pos) == POS_OK)
				break;
		}

		break;

	case 2:
		/* parse either a position string or an endgame-db index */
		parsed_pc = strtoul(argv[1], &endptr, 0);
		if (*endptr == '\0') {
			/* argument looks like a database index */
			if (parsed_pc >= MAX_POS) {
				printf("Poscode too large! Must be smaller than %u!\n", (unsigned)MAX_POS);
				return (EXIT_FAILURE);
			}

			pc = (pos_code)parsed_pc;
			decode_pos(&pos, pc);
		} else {
			/* argument looks like a position string */
			turn = parse_position(&pos, argv[1]);
			if (turn == -1) {
				printf("Position string %s is invalid!\n", argv[1]);
				return (EXIT_FAILURE);
			}

			pc = encode_pos(&pos);
		}

		break;

	case 10:
		pos.c = atoi(argv[1]);
		pos.C = atoi(argv[2]);
		pos.g = atoi(argv[3]);
		pos.G = atoi(argv[4]);
		pos.e = atoi(argv[5]);
		pos.E = atoi(argv[6]);
		pos.l = atoi(argv[7]);
		pos.L = atoi(argv[8]);
		pos.op = strtol(argv[9], NULL, 0);
		if (check_pos(&pos) == POS_INVALID) {
			printf("Position invalid!\n");
			return (EXIT_FAILURE);
		}

		pc = encode_pos(&pos);
		break;

	default:
		printf("usage: %s\n", argv[0]);
		printf("       %s pc\n", argv[0]);
		printf("       %s c C g G e E l L op\n", argv[0]);
		return (EXIT_FAILURE);
	}

	switch (check_pos(&pos)) {
	case POS_OK:
	default:
		printf("PC:  %10u\n", pc);
		break;

	case POS_INVALID:
		printf("PC:  %10u (invalid)\n", pc);
		break;

	case POS_SENTE:
		printf("PC:  %10u (won)\n", pc);
		break;
	}

	printf("POS: ");
	show_pos(&pos);
	printf("\n\n");
	display_pos(&pos);

	printf("\n%s to move.\n", turn ? "Sente" : "Gote");
	move_count = generate_all_moves(moves, &pos);
	printf("Possible moves (%d):\n", move_count);

	for (i = 0; i < move_count; i++) {
		printf("%2d: ", i);
		show_move(&pos, moves[i]);
		putchar(' ');
		display_move(&pos, moves[i]);
	}

	return (EXIT_SUCCESS);
}
