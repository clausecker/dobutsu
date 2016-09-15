#include <stdio.h>
#include <stdlib.h>

#include "dobutsu.h"

struct move_and_distance {
	struct move m;
	short dtm;
};

static void print_moves(const struct position *, int, const struct move_and_distance *, int, GAMEDB *);
static int compare_distance_to_mate(const void*, const void*);
static int analyze_moves(const struct position *, int, struct move_and_distance *, GAMEDB *);

/*
 * The program "bestmove" prints a list of moves for the current
 * position, sorted in descending order by their quality.
 */
extern int
main(int argc, char *argv[])
{
	GAMEDB *gamedb;

	const char *db_filename = "game.db", *pos_str = "S/gle/-c-/-C-/ELG/-";
	struct move_and_distance mad[MAX_MOVES];
	struct position pos;
	int to_move, move_count, move, game_end;
	char line[BUFSIZ], *endptr;

	switch (argc) {
	case 1:
		break;

	case 2:
		pos_str = argv[1];
		break;

	case 3:
		db_filename = argv[1];
		pos_str = argv[2];
		break;

	default:
		fprintf(stderr, "Usage: %s [[game.db] position]\n", argv[0]);
		return (EXIT_FAILURE);
	}

	gamedb = open_gamedb(db_filename);
	if (gamedb == NULL) {
		perror("Cannot open game DB");
		return (EXIT_FAILURE);
	}

	to_move = parse_position(&pos, pos_str);
	if (to_move == TURN_INVALID) {
		fprintf(stderr, "Cannot parse position %s.\n", pos_str);
		return (EXIT_FAILURE);
	}

	if (check_pos(&pos) == POS_INVALID) {
		fprintf(stderr, "Position %s is invalid.\n", pos_str);
		return (EXIT_FAILURE);
	}

	for (;;) {
		display_pos(&pos);

		puts("");
		move_count = analyze_moves(&pos, to_move, mad, gamedb);
		print_moves(&pos, to_move, mad, move_count, gamedb);

		if (move_count == 0) {
			printf("Stalemate! %s loses!\n", to_move ? "Sente" : "Gote");
			break;
		}

		do {
			printf("\nChoose a move to play: ");
			fflush(stdout);
			if (fgets(line, BUFSIZ, stdin) == NULL) {
				if (feof(stdin)) {
					printf("\nGoodbye!\n");
					goto cleanup;
				} else { /* ferror */
					perror("Error reading move");
					exit (EXIT_FAILURE);
				}
			}
		} while (sscanf(line, "%d", &move) != 1 || move <= 0 || move > move_count);

		game_end = apply_move_for(to_move, &pos, mad[move - 1].m);
		if (game_end) {
			printf("\n%s wins!\n", to_move ? "Sente" : "Gote");
			break;
		}

		to_move = !to_move;
		puts("");
	}

cleanup:
	close_gamedb(gamedb);
	return (EXIT_SUCCESS);
}

/*
 * Find all moves from pos with to_move to move, order them by how good
 * they are and store the result in mad.  mad should have enough space
 * for MAX_MOVES entries.
 */
static int
analyze_moves(const struct position *pos, int to_move,
    struct move_and_distance *mad, GAMEDB *db)
{
	struct move moves[MAX_MOVES];
	int move_count, i;

	move_count = generate_all_moves_for(to_move, moves, pos);
	for (i = 0; i < move_count; i++) {
		struct position npos = *pos;
		int game_end;

		mad[i].m = moves[i];
		game_end = apply_move_for(to_move, &npos, mad[i].m);
		if (game_end) {
			mad[i].dtm = POS_INVALID;
			continue;
		}

		mad[i].dtm = distance_to_mate(db, &npos, !to_move);
		if (mad[i].dtm == POS_IOERROR) {
			perror("Error reading game database");
			exit(EXIT_FAILURE);
		}
	}

	qsort(mad, move_count, sizeof *mad, compare_distance_to_mate);

	return (move_count);
}

/*
 * Print out the moves in mad and an evaluation of the position as a
 * whole.
 */
static void
print_moves(const struct position *pos, int to_move,
    const struct move_and_distance *mad, int move_count, GAMEDB *db)
{
	int i;
	int dtm = distance_to_mate(db, pos, to_move);

	switch (dtm) {
	case POS_DRAW:
		printf("Position is a draw. Possible moves:\n");
		break;

	case POS_IOERROR:
		perror("Error reading game database");
		exit(EXIT_FAILURE);

	default:
		printf("Position is a %s in %d. Possible moves:\n", dtm & 1 ? "loss" : "win", dtm + 1);
	}

	for (i = 0; i < move_count; i++) {
		struct position npos = *pos;
		int game_end;
		char movestr[MOVE_LENGTH], posstr[POS_LENGTH];

		move_notation(movestr, pos, mad[i].m, to_move);
		if (mad[i].dtm == POS_INVALID) {
			printf("%2d: %s win\n", i + 1, movestr);
			continue;
		}

		apply_move_for(to_move, &npos, mad[i].m);
		pos_notation(posstr, !to_move, &npos);


		if (mad[i].dtm == POS_DRAW)
			printf("%2d: %s draw:        %s\n", i + 1, movestr, posstr);
		else
			printf("%2d: %s %s in %3d: %s\n", i + 1, movestr,
			    mad[i].dtm & 1 ? "win " : "loss", mad[i].dtm + 1, posstr);
	}
}

/*
 * comparison function such that winning positions are sorted before
 * draws are sorted before losses.
 */
static int
compare_distance_to_mate(const void *ap, const void *bp)
{
	const struct move_and_distance *a = ap, *b = bp;

	if (a->dtm == b->dtm)
		return (0);

	/* a is winning immediately */
	if (a->dtm == POS_INVALID)
		return (-1);

	/* b is winning immediately */
	if (b->dtm == POS_INVALID)
		return (1);

	/* a is drawn */
	if (a->dtm == POS_DRAW)
		return (b->dtm & 1 ? 1 : -1);

	/* b is drawn */
	if (b->dtm == POS_DRAW)
		return (a->dtm & 1 ? -1 : 1);

	/* a is winning */
	if (a->dtm & 1)
		return (b->dtm & 1 ? a->dtm - b->dtm : -1);

	/* b is winning */
	if (b->dtm & 1)
		return (1);

	/* here both must be losing */
	return (b->dtm - a->dtm);
}
