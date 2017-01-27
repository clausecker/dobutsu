#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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
	ENGINE_BOTH = 3
};

/* global variables */
static struct tablebase *tb = NULL;
static struct gamestate *gs = NULL;
static unsigned char engine_players = 0;
static double sente_strength = 1, gote_strength = 1;
static struct seed seed;
static size_t linebuflen;
static char *linebuf = NULL;

/* internal functions */
static void	open_tablebase(void);
static void	execute_command(char *);
static void	end_game(void);
static void	cmd_hint(const char *);
static void	cmd_new(const char *);
static void	cmd_exit(const char *);
static void	cmd_show(const char *);
static void	cmd_show_board(void);
static void	cmd_show_moves(void);
static void	cmd_show_eval(void);
static void	cmd_show_lines(void);
static void	cmd_strength(const char *);
static void	cmd_undo(const char *);
static void	cmd_remove(const char *);
static void	cmd_help(const char *);
static void	cmd_version(const char *);
static void	cmd_both(const char *);
static void	cmd_go(const char *);
static void	cmd_force(const char *);
static int	play(struct move m);
static void	prompt(void);
static void	autoplay(void);
static int	undo(void);
static int	draw(void);

/*
 * This table contains all available commands.  New commands should be
 * added to this table.  The callback function takes as its sole
 * argument a pointer to the remainder of the command line.
 * The table is terminated with an entry containing NULL for the
 * function pointer.
 */
static const struct {
	void (*callback)(const char *);
	char command[8];
} commands[] = {
	cmd_both,	"both",
	cmd_exit,	"exit",
	cmd_force,	"force",
	cmd_go,		"go",
	cmd_help,	"help",
	cmd_hint,	"hint",
	cmd_new,	"new",
	cmd_exit,	"quit",
	cmd_remove,	"remove",
	cmd_new,	"setup",
	cmd_new,	"setboard",
	cmd_show,	"show",
	cmd_strength,	"strength",
	cmd_undo,	"undo",
	cmd_version,	"version",
	NULL,		""
};

/*
 * Similar to the previous table, this table contains all subcommands
 * for the show command.
 */
static const struct {
	void (*callback)(void);
	char command[8];
} show_commands[] = {
	cmd_show_board,	"board",
	cmd_show_moves, "moves",
	cmd_show_eval,	"eval",
	cmd_show_lines,	"lines",
	NULL,		""
};

extern int
main(int argc, char *argv[])
{

	/* TODO */
	(void)argc;
	(void)argv;

	/* for better interaction */
	setbuf(stdin, NULL);
	setbuf(stdout, NULL);

	ai_seed(&seed);
	open_tablebase();
	cmd_new("");

	prompt();
	while (getline(&linebuf, &linebuflen, stdin) > 0) {
		execute_command(linebuf);
		prompt();
	}

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

	printf("Loading tablebase... ");

	if (tbloc != NULL)
		tbfile = fopen(tbloc, "rb");
	else {
		tbloc = "dobutsu.tb";
		tbfile = fopen(tbloc, "rb");
		if (tbfile == NULL && errno == ENOENT) {
			tbloc = "dobutsu.tb.xz";
			tbfile = fopen(tbloc, "rb");
		}
	}

	if (tbfile != NULL) {
		tb = read_tablebase(tbfile);
		fclose(tbfile);
	}

	if (tb != NULL)
		puts("done");
	else
		printf("%s: %s\n", tbloc, errno == 0 ? "Unknown error" : strerror(errno));
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
 * with the state of a new game.  If the first argument is empty, use
 * the default setup.  If the first argument is not empty, try to setup
 * the board with the specified setup.
 */
static void
cmd_new(const char *arg)
{
	struct position p;

	if (arg[0] == '\0')
		p = INITIAL_POSITION;
	else if (parse_position(&p, arg) != 0) {
		printf("Error (invalid position): %s\n", arg);
		return;
	}

	end_game();
	engine_players = ENGINE_NONE;
	gs = malloc(sizeof *gs);
	if (gs == NULL) {
		perror("malloc");
		exit(EXIT_FAILURE);
	}

	gs->previous = NULL;
	gs->position = p;
	gs->move_clock = 1;
}

/*
 * Execute the command given in cmd.
 */
static void
execute_command(char *cmd)
{
	struct move m;
	size_t i;
	char *arg;

	/* trim newline if any */
	cmd[strcspn(cmd, "\r\n")] = '\0';


	/* trim leading whitespace */
	cmd += strspn(cmd, " ");

	/* if a move is given, try to play that move */
	if (parse_move(&m, &gs->position, cmd) == 0) {
		if (play(m)) {
			puts("You win!\nStarting new game.");
			cmd_new("");
		} else if (draw()) {
			puts("Draw by threefold repetition.\nStarting new game.");
			cmd_new("");
		}

		autoplay();
		return;
	}

	/* else, split command at the first whitespace */
	arg = cmd + strcspn(cmd, " ");
	if (*arg != '\0')
		*arg++ = '\0';

	for (i = 0; commands[i].callback != NULL; i++) {
		if (strncmp(commands[i].command, cmd, sizeof commands[i].command) != 0)
			continue;

		commands[i].callback(arg);
		return;
	}

	printf("Error (unknown command): %s\n", cmd);
}

/*
 * Play m on the current game state and update gs appropriately.
 * Return nonzero if the game ended through that move.
 */
static int
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

	return (play_move(&gs->position, m));
}

/*
 * Return nonzero if it's the engine's turn.
 */
static int
engine_moves(void)
{
	return engine_players &
	    (gote_moves(&gs->position) ? ENGINE_GOTE : ENGINE_SENTE);
}

/*
 * If it's the engine's turn, play a move for the engine.  Repeat until
 * either it's no longer the engine's turn or (if both players are
 * engine players) until the game ends.
 */
static void
autoplay(void)
{
	struct move engine_move;
	double strength;
	int end;
	unsigned old_clock;
	char movstr[MAX_MOVSTR];

	while (engine_moves()) {
		strength = gote_moves(&gs->position) ? gote_strength : sente_strength;

		engine_move = ai_move(tb, &gs->position, &seed, strength);
		move_string(movstr, &gs->position, engine_move);
		old_clock = gs->move_clock;
		end = play(engine_move);
		cmd_show_board();
		printf("\nMy %u. move is : %s\n", old_clock, movstr);
		if (end) {
			puts("I win!\nStarting new game.");
			cmd_new("");
		} else if (draw()) {
			puts("Draw by threefold repetition.\nStarting new game.");
			cmd_new("");
		}
	}
}

/*
 * Generate an engine move for the current colour and print it out.
 */
static void
cmd_hint(const char *arg)
{
	double strength = gote_moves(&gs->position) ? gote_strength : sente_strength;
	char movstr[MAX_MOVSTR];

	(void)arg;

	if (tb == NULL) {
		printf("Error (tablebase unavailable): hint\n");
		return;
	}

	move_string(movstr, &gs->position, ai_move(tb, &gs->position, &seed, strength));
	puts(movstr);
}

/*
 * Close everything and exit the program.  This function should not
 * return.  The first argument is ignored.
 */
static void
cmd_exit(const char *arg)
{

	(void)arg;

	end_game();
	free_tablebase(tb);
	tb = NULL;
	free(linebuf);

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
	size_t i;

	for (i = 0; show_commands[i].callback != NULL; i++) {
		if (strcmp(arg, show_commands[i].command) != 0)
			continue;

		show_commands[i].callback();
		return;
	}

	printf("Error (unknown command): show %s\n", arg);
}

/*
 * Print the current board to stdout.
 */
static void
cmd_show_board()
{
	char render[MAX_RENDER];

	position_render(render, &gs->position);
	fputs(render, stdout);
}

/*
 * Print all possible moves to stdout.
 */
static void
cmd_show_moves()
{
	struct move moves[MAX_MOVES];
	size_t i, n;
	char movstr[MAX_MOVSTR];

	n = generate_moves(moves, &gs->position);
	for (i = 0; i < n; i++) {
		move_string(movstr, &gs->position, moves[i]);
		puts(movstr);
	}
}

/*
 * Lookup the current position in the tablebase and print its
 * evaluation.
 */
static void
cmd_show_eval(void)
{
	tb_entry eval;

	if (tb == NULL) {
		printf("Error (tablebase unavailable): show eval");
		return;
	}

	eval = lookup_position(tb, &gs->position);
	if (is_win(eval))
		printf("#%d\n", get_dtm(eval));
	else if (is_loss(eval))
		printf("#-%d\n", get_dtm(eval));
	else /* is_draw(eval) */
		printf("0\n");
}

/*
 * For each move, print its evaluation and engine probability.
 */
static void
cmd_show_lines(void)
{
	struct analysis analysis[MAX_MOVES];
	double strength = gote_moves(&gs->position) ? gote_strength : sente_strength;
	size_t i, nmove;
	char movstr[MAX_MOVSTR], dtmstr[6];

	if (tb == NULL) {
		printf("Error (tablebase unavailable): show lines");
		return;
	}

	nmove = analyze_position(analysis, tb, &gs->position, strength);
	for (i = 0; i < nmove; i++) {
		move_string(movstr, &gs->position, analysis[i].move);
		if (is_draw(analysis[i].entry))
			strcpy(dtmstr, "0");
		else
			snprintf(dtmstr, sizeof dtmstr,
			    is_win(analysis[i].entry) ? "#%d" : "#-%d",
			    get_dtm(analysis[i].entry));

		printf("%-7s: %-5s (%5.2f%%)\n", movstr, dtmstr, analysis[i].value * 100.0);
	}
}

/*
 * The strength command lets you set the engine strength.  If no operand
 * is provided, the current engine strength is printed.  If one operand
 * is provided, both engine's strengths are set to that value.  If two
 * operands are provided, Sente's and Gote's strengths are set to the
 * two values.
 */
static void
cmd_strength(const char *arg)
{
	double s, g;

	switch (sscanf(arg, "%lf%lf", &s, &g)) {
	case 1:
		g = s;
		/* fallthrough */

	case 2:
		/* also catch NaN */
		if (!(s >= 0 && g >= 0)) {
			printf("Error (strength must be positive): strength %s\n", arg);
			return;
		}

		sente_strength = s;
		gote_strength = g;
		break;

	default:
		printf("Sente: %6.2f\nGote:  %6.2f\n", sente_strength, gote_strength);
		break;
	}
}

/*
 * The undo command lets the player undo a single move.
 */
static void
cmd_undo(const char *arg)
{

	(void)arg;

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

	(void)arg;

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
 * Print a list of commands. Argument is ignored.
 */
static void
cmd_help(const char *arg)
{

	(void)arg;

	printf(
	    "help        Print a list of commands\n"
	    "hint        Print what the engine would play\n"
	    "quit        Quit the program\n"
	    "exit        Quit the program\n"
	    "version     Print program version\n"
	    "new         Start a new game\n"
	    "undo        Undo previous move\n"
	    "remove      Undo last two moves\n"
	    "setup       Setup board with specified postion string\n"
	    "show board  Print the current board\n"
	    "show moves  Print all possible moves\n"
	    "show eval   Print position evaluation\n"
	    "show lines  Print all possible moves and their evaluations\n"
	    "strength    Show/set engine strength\n"
	    "both        Make engine play both players\n"
	    "go          Make the engine play the colour that is on the move\n"
	    "force       Set the engine to play neither colour\n");
}

/*
 * Print the program version.  The version can be set using the VERSION
 * macro.  If VERSION is unset, "unknown" is assumed. The argument is
 * ignored.  Argument is ignored.
 */
static void
cmd_version(const char *arg)
{

	(void)arg;
	printf("dobutsu %s\n",
#ifndef VERSION
	    "unknown");
#else
	    VERSION);
#endif
}

/*
 * Make the program play whatever players move it currently is and make
 * the human take the other colour.  Then play a move.  The argument is
 * ignored.
 */
static void
cmd_go(const char *arg)
{

	(void)arg;

	engine_players = gote_moves(&gs->position) ? ENGINE_GOTE : ENGINE_SENTE;
	autoplay();
}

/*
 * Set both engines to run, arg is ignored.
 */
static void
cmd_both(const char *arg)
{

	(void)arg;

	engine_players = ENGINE_BOTH;
	autoplay();
}

/*
 * Set both players to manual.  The argument is ignored.
 */
static void
cmd_force(const char *arg)
{

	(void)arg;

	engine_players = ENGINE_NONE;
}

/*
 * Print a prompt of the form "##. " where ## is the current move
 * number.
 */
static void
prompt(void)
{

	printf("%u. ", gs->move_clock);
	fflush(stdout);
}

/*
 * Check if the current position is a draw by threefold repetition.
 * Return nonzero if it is, zero otherwise.
 */
static int
draw(void)
{
	struct position *p = &gs->position;
	struct gamestate *gsptr;
	int repetitions = 1;

	for (gsptr = gs->previous; gsptr != NULL; gsptr = gsptr->previous)
		if (position_equal(p, &gsptr->position) && ++repetitions == 3)
			return (1);


	return (0);
}
