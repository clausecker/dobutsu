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
	int move_count, i, turn = TURN_SENTE;
	char *endptr, posbuf[POS_LENGTH], movebuf[MOVE_LENGTH];
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

		turn = rand() & 1;
		if (turn == TURN_GOTE)
			turn_position(&pos);

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

			if (turn == TURN_SENTE)
				pc = encode_pos_check(&pos);
			else {
				struct position fpos = pos;
				turn_position(&fpos);
				pc = encode_pos_check(&fpos);
			}
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

		pc = encode_pos_check(&pos);
		break;

	default:
		printf("usage: %s\n", argv[0]);
		printf("       %s pc\n", argv[0]);
		printf("       %s c C g G e E l L op\n", argv[0]);
		return (EXIT_FAILURE);
	}

	printf("Poscode:  %u", pc);
	switch (check_pos(&pos)) {
	case POS_OK:
	default:
		puts("");
		break;

	case POS_INVALID:
		puts(" (invalid)");
		break;

	case POS_SENTE:
		puts(" (won)");
		break;
	}

	pos_notation(posbuf, turn, &pos);
	printf("Position: %s\nInternal: ", posbuf);
	show_pos(&pos);
	printf("\n\n");
	display_pos(&pos);

	printf("\n%s to move.\n", turn ? "Sente" : "Gote");
	move_count = generate_all_moves_for(turn, moves, &pos);
	printf("Possible moves (%d):\n", move_count);

	for (i = 0; i < move_count; i++) {
		move_notation(movebuf, &pos, moves[i]);
		printf("%2d: %s ", i, movebuf);
		describe_move(&pos, moves[i]);
	}

	return (EXIT_SUCCESS);
}
