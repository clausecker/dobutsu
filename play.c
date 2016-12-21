/* play a game of Dobutsu Shogi with yourself */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "rules.h"

extern int
main(int argc, char *argv[])
{
	struct position p = INITIAL_POSITION;
	struct move moves[MAX_MOVES], user_move;
	size_t move_count, i;
	char buffer[1000];

	if (argc > 1 && parse_position(&p, argv[1]) == -1) {
		printf("position %s invalid!\n", argv[1]);
		return (EXIT_FAILURE);
	}

	for (;;) {

		position_string(buffer, &p);
		printf("%s\n", buffer);
		position_render(buffer, &p);
		printf("%s\n", buffer);
		assert(position_valid(&p));

		if (!gote_moves(&p) ? sente_in_check(&p) : gote_in_check(&p))
			printf("You are in check.\n");

		move_count = generate_moves(moves, &p);
		for (i = 0; i < move_count; i++) {
			move_string(buffer, &p, moves[i]);
			printf("%2zu: %s\n", i, buffer);
			assert(move_valid(&p, moves[i]));
		}

		for (;;) {
			printf("Select a move: ");
			fflush(stdout);
			if (fgets(buffer, sizeof buffer, stdin) == NULL) {
				if (feof(stdin))
					puts("bye!");
				else
					perror("error reading stdin");

				return (EXIT_FAILURE);
			}

			/* check if move is a number */
			if (sscanf(buffer, "%zu", &i) == 1) {
				user_move = moves[i];
				break;
			}

			if (parse_move(&user_move, &p, buffer) == 0)
				break;
		}

		if (play_move(&p, user_move)) {
			printf("You won!\n");
			return (EXIT_SUCCESS);
		}
	}
}
