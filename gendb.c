/* gendb -- generate the Dobutsu Shogi endgame database */
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>

#include "dobutsu.h"

static unsigned	init_round(unsigned char *restrict);
static unsigned gote_round(unsigned char *restrict, unsigned);
static unsigned sente_round(unsigned char *restrict, unsigned);

extern int
main(int argc, char *argv[])
{
	unsigned char *db;
	unsigned count, round;
	int dbfd;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s endgame.db\n", argv[0]);
		return (EXIT_FAILURE);
	}

	dbfd = open(argv[1], O_RDWR|O_CREAT, 0666);
	if (dbfd == -1) {
		perror("Cannot create database");
		return (EXIT_FAILURE);
	}

	if (ftruncate(dbfd, MAX_POS) == -1) {
		perror("Cannot truncate database");
		close(dbfd);
		return (EXIT_FAILURE);
	}

	db = mmap(NULL, MAX_POS, PROT_WRITE, MAP_SHARED, dbfd, 0);
	if (db == MAP_FAILED) {
		perror("Cannot map database");
		close(dbfd);
		return (EXIT_FAILURE);
	}

	fprintf(stderr, "round  0 (init)  ... ");
	fprintf(stderr, "%10u\n", init_round(db));

	for (round = 1, count = -1; count != 0; round++) {
		fprintf(stderr, "round %2d %s ... ", round, round % 2 ? "(gote) " : "(sente)");
		count = round % 2 ? gote_round(db, round) : sente_round(db, round);
		fprintf(stderr, "%10u\n", count);
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

		/* count stalemate as a draw */
		if (move_count == 0)
			continue;

		for (j = 0; j < move_count; j++) {
			newp = p;
			apply_move(&newp, moves[j]);
			turn_position(&newp);
			newpc = encode_pos(&newp);

			if (newpc == (pos_code)POS_SENTE)
				continue;

			assert(newpc < MAX_POS);

			/* search for a move that does not loose */
			if (db[newpc] == 0xfe)
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

		/* count stalemate as a draw */
		if (move_count == 0)
			continue;

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
