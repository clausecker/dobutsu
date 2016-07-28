#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "dobutsu.h"

/* program to test display.c */
extern int main(int argc, char *argv[])
{
	struct position pos;
	pos_code pc;

	switch (argc) {
	case 1:
		srand(time(NULL));
		do pc = rand();
		while (decode_pos(&pos, pc), check_pos(&pos) != POS_OK);
		break;

	case 2:
		pc = (pos_code)atol(argv[1]);
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
		pc = encode_pos(&pos);
		break;

	default:
		printf("usage: %s\n", argv[0]);
		printf("       %s pc\n", argv[0]);
		printf("       %s c C g G e E l L op\n", argv[0]);
		return (EXIT_FAILURE);
	}

		if (pc >= MAX_POS) {
			printf("Poscode to large! Must be smaller than %u!\n", (unsigned)MAX_POS);
			return (EXIT_FAILURE);
		}

		decode_pos(&pos, pc);
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

	return (EXIT_SUCCESS);
}
