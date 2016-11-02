/* play a game of Dobutsu Shogi with yourself */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "rules.h"

extern int
main()
{
	struct position p = INITIAL_POSITION;
	struct move moves[MAX_MOVES];
	size_t move_count, i;
	char buffer[1000];

	for (;;) {

		position_string(buffer, &p);
		printf("%s\n", buffer);
		position_render(buffer, &p);
		printf("%s\n", buffer);
		assert(position_valid(&p));

		move_count = generate_moves(moves, &p);
		for (i = 0; i < move_count; i++) {
			move_string(buffer, &p, moves[i]);
			printf("%2zu: %s\n", i, buffer);
			assert(move_valid(&p, moves[i]));
		}
		do {
			printf("Select a move: ");
			fflush(stdout);
		} while (i = 0, scanf("%zu", &i) == 0 || i >= move_count);
		if (feof(stdin)) {
			printf("bye!\n");
			return (EXIT_SUCCESS);
		} else if (ferror(stdin)) {
			perror("error reading stdin");
			return (EXIT_FAILURE);
		}

		if (play_move(&p, moves[i])) {
			printf("You won!\n");
			return (EXIT_SUCCESS);
		}
	}
}
