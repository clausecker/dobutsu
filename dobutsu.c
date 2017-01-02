#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include "rules.h"
#include "tablebase.h"

/*
 * The struct game represents the entire state of the game.
 */
struct gamestate {
	struct gamestate *previous;
	struct position position;
	struct move current_move;
	unsigned move_clock;
};

/* global variables */
static struct tablebase *tb = NULL;
static struct gamestate *gs = NULL;

/* internal functions */
static void	open_tablebase(void);
static void	end_game(void);
static void	new_game(void);
static void	execute_command(char *);

extern int
main(int argc, char *argv[])
{
	size_t linebuflen = 0;
	char *linebuf = 0;

	/* TODO */
	(void)argc;
	(void)argv;

	/* for XBoard compatibility */
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);

	open_tablebase();
	new_game();

	while (getline(&linebuf, &linebuflen, stdin) > 0)
		execute_command(linebuf);

	end_game();
	free_tablebase(tb);
	tb = NULL;

	if (ferror(stdin)) {
		perror("Error reading command");
		return (EXIT_FAILURE);
	}

	return (EXIT_SUCCESS);
}

/*
 * Open the endgame tablebase whose location should be described by the
 * environment variable DOBUTSU_TABLEBASE.  If that variable is unset or
 * no tablebase is found, try opening a file named dobutsu.tb in the
 * current working directory.  If that doesn't work either, give up.
 */
static void
open_tablebase()
{
	FILE *tbfile;
	const char *tbloc = getenv("DOBUTSU_TABLEBASE");

	if (tbloc == NULL)
		tbloc = "dobutsu.tb";

	tbfile = fopen(tbloc, "rb");
	if (tbfile == NULL) {
		fprintf(stderr, "Cannot open tablebase %s: ", tbloc);
		perror(NULL);
		return;
	}

	printf("Loading tablebase...");
	tb = read_tablebase(tbfile);
	if (tb == NULL) {
		printf("\n");
		fprintf(stderr, "Error loading tablebase from file %s: ", tbloc);
		perror(NULL);
	} else
		printf(" done.\n");
}

/*
 * Deallocate gs and the entire undo-state corresponding to it.
 */
static void
end_game()
{
	struct gamestate *gsptr = gs, *prevgs;

	while (gsptr != NULL) {
		prevgs = gsptr->previous;
		free(gsptr);
		gsptr = prevgs;
	}
}

/*
 * Start a new game by clearing the old game state and initializing it
 * with the state of a new game.
 */
static void
new_game()
{

	end_game();
	gs = malloc(sizeof *gs);
	if (gs == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	gs->previous = NULL;
	gs->position = INITIAL_POSITION;
	gs->move_clock = 0;
}

/*
 * Execute the command given in cmd.
 */
static void
execute_command(char *cmd)
{
	printf("Error (unknown command): %s", cmd);
}
