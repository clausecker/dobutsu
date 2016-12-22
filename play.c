/* play a game of Dobutsu Shogi with yourself */

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "rules.h"
#include "tablebase.h"

extern int
main(int argc, char *argv[])
{
	struct tablebase *tb = NULL;
	struct position p = INITIAL_POSITION, pp;
	struct move moves[MAX_MOVES], user_move;
	struct unmove unmoves[MAX_UNMOVES];
	FILE *tbfile;
	size_t move_count, unmove_count, i;
	char buffer[1000];

	if (argc > 1 && parse_position(&p, argv[1]) == -1) {
		printf("position %s invalid!\n", argv[1]);
		return (EXIT_FAILURE);
	}

	tbfile = fopen("game.db", "rb");
	if (tbfile != NULL) {
		tb = read_tablebase(tbfile);
		fclose(tbfile);
	}

	if (tb == NULL)
		printf("Tablebase not loaded: %s\n", strerror(errno));

	for (;;) {

		position_string(buffer, &p);
		printf("%s\n\n", buffer);
		position_render(buffer, &p);
		printf("%s\n", buffer);
		assert(position_valid(&p));

		if (tb != NULL) {
			poscode pc;
			tb_entry value;
			struct position pp;

			encode_position(&pc, &p);
			value = lookup_poscode(tb, pc);

			decode_poscode(&pp, pc);
			position_string(buffer, &pp);
			printf("%s\n", buffer);

			if (is_win(value))
				printf("Win in %d.\n", get_dtm(value));
			else if (is_loss(value))
				printf("Loss in %d.\n", get_dtm(value));
			else
				printf("Draw.\n");
		}

		if (!gote_moves(&p) ? sente_in_check(&p) : gote_in_check(&p))
			printf("You are in check.\n");

		unmove_count = generate_unmoves(unmoves, &p);
		for (i = 0; i < unmove_count; i++) {
			pp = p;
			undo_move(&pp, unmoves[i]);
			assert(position_valid(&pp));
			position_string(buffer, &pp);
			printf("%2zu: (%2u, %2u, %2u, %2d) %s\n", i,
			    unmoves[i].piece, unmoves[i].from, unmoves[i].status, unmoves[i].capture,
			    buffer);
		}

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
