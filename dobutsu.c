#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
	struct move next_move;
	unsigned move_clock;
};

/*
 * The engine may play any combination of Sente and Gote, including
 * no player.
 */
enum {
	ENGINE_NONE = 0,
	ENGINE_SENTE = 1,
	ENGINE_GOTE = 2,
	ENGINE_SENTE_GOTE = 3
};

/* global variables */
static struct tablebase *tb = NULL;
static struct gamestate *gs = NULL;

static unsigned char engine_players = 0;

/* internal functions */
static void	open_tablebase(void);
static void	execute_command(char *);
static void	end_game(void);
static void	new_game(const char *);
static void	cmd_exit(const char *);
static void	cmd_show(const char *);
static void	cmd_undo(const char *);
static void	cmd_remove(const char *);
static void	cmd_help(const char *);
static void	cmd_version(const char *);
static void	cmd_nop(const char *);
static void	cmd_xboard(const char *);
static void	play(struct move m);
static void	autoplay(void);
static int	undo(void);

/*
 * This table contains all available commands.  New commands should be
 * added to this table.  The callback function takes as its sole
 * argument the second word of the command.  The member words indicates
 * how many words a command of this kind takes.  If a wrong number of
 * words is passed, an error is produced instead.  The table is
 * terminated with an entry containing NULL for the function pointer.
 */
static const struct {
	void (*callback)(const char *);
	unsigned char words;
	char command[11];
} commands[] = {
	cmd_exit,	1, "exit",
	cmd_help,	1, "help",
	new_game,	1, "new",
	cmd_exit,	1, "quit",
	cmd_nop,	1, "random",
	cmd_remove,	1, "remove",
	cmd_show,	2, "show",
	cmd_undo,	1, "undo",
	cmd_version,	1, "version",
	cmd_xboard,	1, "xboard",
	/* unimplemented commands */
/*	cmd_hint,	1, "hint",
	cmd_go,		1, "go",
	cmd_force,	1, "force",
	cmd_setboard,	2, "setboard",
	cmd_variant,	2, "variant",
	cmd_white,	1, "white",
	cmd_black,	1, "black",
	cmd_playother,	1, "playother",
	cmd_level,	4, "level",
	cmd_st,		1, "st",
	cmd_sd,		2, "sd",
	cmd_ping,	2, "ping",
*/
	NULL,		0, "",
};

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
	new_game(NULL);

	while (getline(&linebuf, &linebuflen, stdin) > 0)
		execute_command(linebuf);

	cmd_exit(NULL);
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
 * with the state of a new game.  The first argument must be NULL.
 */
static void
new_game(const char *arg)
{

	assert(arg == NULL);

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
	struct move m;
	size_t i;
	unsigned words;
	char *context, *word1, *word2, *word3;

	/* trim newline if any */
	cmd[strcspn(cmd, "\r\n")] = '\0';

	if (parse_move(&m, &gs->position, cmd) == 0) {
		play(m);
		autoplay();
		return;
	}

	word1 = strtok_r(cmd, " ", &context);
	word2 = strtok_r(NULL, " ", &context);
	word3 = strtok_r(NULL, " ", &context);

	if (word1 == NULL)
		return;
	else if (word2 == NULL)
		words = 1;
	else if (word3 == NULL)
		words = 2;
	else
		words = 3;

	for (i = 0; commands[i].callback != NULL; i++) {
		if (strcmp(commands[i].command, word1) != 0 || words != commands[i].words)
			continue;

		commands[i].callback(word2);
		return;
	}

	printf("Error (unknown command): %s\n", cmd);
}

/*
 * Play m on the current game state and update gs appropriately.
 */
static void
play(struct move m)
{
	struct gamestate *newgs = malloc(sizeof *newgs);

	if (newgs == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	newgs->previous = gs;
	newgs->position = gs->position;
	newgs->move_clock = gs->move_clock + 1;
	gs->next_move = m;
	gs = newgs;

	if (play_move(&gs->position, m)) {
		printf("You won!\nStarting new game.\n");
		new_game(NULL);
	}
}

/*
 * Close everything and exit the program.  This function should not
 * return.  The first argument is ignored.
 */
static void
cmd_exit(const char *arg)
{

	assert(arg == NULL);

	end_game();
	free_tablebase(tb);
	tb = NULL;

	if (ferror(stdin)) {
		perror("Error reading command");
		exit(EXIT_FAILURE);
	}

	exit(EXIT_SUCCESS);
}

/*
 * The show command has several subcommands.  Due to the small number of
 * subcommands, we call them directly through an if-cascade.
 */
static void
cmd_show(const char *arg)
{
	struct move moves[MAX_MOVES];
	size_t i, n;
	char render[MAX_RENDER], movstr[MAX_MOVSTR];

	assert(arg != NULL);

	if (strcmp(arg, "board") == 0) {
		position_render(render, &gs->position);
		fputs(render, stdout);
	} else if (strcmp(arg, "moves") == 0) {
		n = generate_moves(moves, &gs->position);
		for (i = 0; i < n; i++) {
			move_string(movstr, &gs->position, moves[i]);
			puts(movstr);
		}
	} else
		printf("Error (unknown command): show %s\n", arg);
}

/*
 * The undo command lets the player undo a single move.
 */
static void
cmd_undo(const char *arg)
{

	assert(arg == NULL);
	if (undo())
		autoplay();
}

/*
 * The remove command lets the player undo two moves.  This is
 * useful when you are playing against the engine.
 */
static void
cmd_remove(const char *arg)
{

	assert(arg == NULL);
	if (undo() && undo())
		autoplay();
}

/*
 * undo undoes the last move played if such a move existed.  This
 * function returns nonzero if a move to be undone exists.
 */
static int
undo(void)
{
	struct gamestate *oldgs = gs;

	if (oldgs->previous != NULL) {
		gs = oldgs->previous;
		free(oldgs);
		return (1);
	} else {
		printf("Nothing to undo.\n");
		return (0);
	}
}

/*
 * If it's the engine's turn, play a move for the engine.  Repeat until
 * either it's no longer the engine's turn or (if both players are
 * engine players) until the game ends.
 */
static void
autoplay(void)
{

	/* TODO */
	;
}

/*
 * Print a list of commands. Argument is ignored.
 */
static void
cmd_help(const char *arg)
{

	assert(arg == NULL);
	printf(
	    "help        Print a list of commands\n"
	    "hint        Print what the engine would play\n"
	    "quit        Quit the program\n"
	    "exit        Quit the program\n"
	    "version     Print program version\n"
	    "new         Start a new game\n"
	    "undo        Undo previous move\n"
	    "remove      Undo last two moves\n"
	    "show board  Print the current board\n"
	    "show moves  Print all possible moves\n"
	    "show eval   Print position evaluation\n"
	    "show lines  Print all possible moves and their evaluations\n"
	    "go          Make the engine play the colour that is on the move\n"
	    "force       Set the engine to play neither colour\n"
	    "setboard    Set the board to the given position string\n");
}

/*
 * Print the program version.  The version can be set using the VERSION
 * macro.  If VERSION is unset, "unknown" is assumed. The argument is
 * ignored.  Argument is ignored.
 */
static void
cmd_version(const char *arg)
{

	assert(arg == NULL);
	printf("dobutsu %s\n",
#ifndef VERSION
	    "unknown");
#else
	    VERSION);
#endif
}

/*
 * The nop command does nothing and ignores its argument.
 */
static void
cmd_nop(const char *arg)
{

	(void)arg;
}

/*
 * The xboard command sets the engine up for xboard mode and then
 * prints a newline.  Since there is nothing to set up, we just
 * print a newline.  arg is ignored.
 */
static void
cmd_nop(const char *arg)
{

	assert(arg == NULL);
	puts("");
}
