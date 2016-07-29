/* gendb -- generate the Dobutsu Shogi endgame database */
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "dobutsu.h"

static unsigned	init_round(unsigned char *restrict);
static unsigned gote_round(unsigned char *restrict, unsigned);
static unsigned sente_round(unsigned char *restrict, unsigned);

extern int
main(int argc, char *argv[])
{
	FILE *dbfile;
	unsigned char *db;
	unsigned count, round, i, draw, invalid, won = 0, lost = 0;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s endgame.db\n", argv[0]);
		return (EXIT_FAILURE);
	}

	dbfile = fopen(argv[1], "w");
	if (dbfile == NULL) {
		perror("Cannot create database");
		return (EXIT_FAILURE);
	}

	db = malloc(MAX_POS);
	if (db == NULL) {
		perror("Cannot allocate memory for database");
	}

	fprintf(stderr, "round   0 (init)  ... ");
	fprintf(stderr, "%10u\n", (won = init_round(db)));

	for (round = 1, count = -1; count != 0; round++) {
		fprintf(stderr, "round %3d %s ... ", round, round % 2 ? "(gote) " : "(sente)");
		if (round % 2)
			lost += count = gote_round(db, round);
		else
			won += count = sente_round(db, round);

		fprintf(stderr, "%10u\n", count);
	}

	/* count number of invalid and unsettled positions */
	for (i = draw = invalid = 0; i < MAX_POS; i++)
		switch (db[i]) {
		case 0xff:
			invalid++;
			break;

		case 0xfe:
			draw++;
			break;

		default:
			;
		}

	fprintf(stderr, "\ninvalid positions: %10u (%4.2f%%)\n", invalid, invalid*100.0/MAX_POS);
	fprintf(stderr, "drawn   positions: %10u (%4.2f%%)\n", draw, draw*100.0/MAX_POS);
	fprintf(stderr, "lost    positions: %10u (%4.2f%%)\n", lost, lost*100.0/MAX_POS);
	fprintf(stderr, "won     positions: %10u (%4.2f%%)\n", won,  won *100.0/MAX_POS);

	if (fwrite(db, 1, MAX_POS, dbfile) != MAX_POS) {
		perror("Error writing database to file");
		return (EXIT_FAILURE);
	}

	return (EXIT_SUCCESS);
}

/*
 * This code performs the initial pass through the database, setting up
 * the indices to 0xff for invalid positions, 0x00 for won positions and
 * 0xfe for undecided positions. This function returns how many won
 * positions have been found.
 */
static unsigned
init_round(unsigned char *restrict db)
{
	struct position p;
	pos_code i;
	unsigned count = 0;

	for (i = 0; i < MAX_POS; i++) {
		decode_pos(&p, i);
		switch (check_pos(&p)) {
		case POS_INVALID:
			db[i] = 0xff;
			break;

		case POS_SENTE:
			db[i] = 0x00;
			count++;
			break;

		case POS_OK:
			db[i] = 0xfe;
			break;

		default:
			assert(0);
		}
	}

	return (count);
}

/*
 * Every odd numbered round is a Gote round because sente is not able to
 * score a win in such a round.  In a Gote round we check if there is
 * any move that leads to a white win. If there is none, we declare the
 * position lost.
 */
static unsigned
gote_round(unsigned char *restrict db, unsigned r)
{
	struct position p, newp;
	struct move moves[MAX_MOVES];
	pos_code i, newpc;
	unsigned j, count = 0, move_count;

	for (i = 0; i < MAX_POS; i++) {
		/* skip already analyzed positions */
		if (db[i] != 0xfe)
			continue;

		decode_pos(&p, i);
		move_count = generate_moves(moves, &p);

		/* stalemate, i.e. move_count == 0 means loss */

		for (j = 0; j < move_count; j++) {
			newp = p;
			apply_move(&newp, moves[j]);
			turn_position(&newp);
			newpc = encode_pos(&newp);

			if (newpc == (pos_code)POS_SENTE)
				continue;

			assert(newpc < MAX_POS);

			/* search for a move that does not loose */
			/* and pretend that each position is evaluated independently */
			if (db[newpc] == 0xfe || db[newpc] == r)
				goto found_move;
		}

		/* no good move found */
		db[i] = r;
		count++;
		continue;

	found_move:
		;
	}

	return (count);
}

static unsigned
sente_round(unsigned char *restrict db, unsigned r)
{
	struct position p, newp;
	struct move moves[MAX_MOVES];
	pos_code i, newpc;
	unsigned j, count = 0, move_count;

	for (i = 0; i < MAX_POS; i++) {
		/* skip already analyzed positions */
		if (db[i] != 0xfe)
			continue;

		decode_pos(&p, i);
		move_count = generate_moves(moves, &p);


		/* stalemate shouldn't happen here */
		for (j = 0; j < move_count; j++) {
			newp = p;
			apply_move(&newp, moves[j]);
			turn_position(&newp);
			newpc = encode_pos(&newp);
			if (newpc == (pos_code)POS_SENTE)
				continue;

			assert(newpc < MAX_POS);

			/* check for a move that wins */
			if (db[newpc] == r - 1)
				goto found_move;
		}

		/* no move found */
		continue;

	found_move:
		db[i] = r;
		count++;
	}

	return (count);
}
