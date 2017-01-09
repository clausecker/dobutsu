#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <errno.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "dobutsutable.h"

static void	*gentb_worker(void *);
static void	 initial_round_chunk(struct tablebase *, poscode, unsigned *, unsigned *);
static void	 initial_round_pos(struct tablebase *, poscode, unsigned *, unsigned *);
static void	 normal_round_chunk(struct tablebase *, poscode, unsigned *, unsigned *, unsigned);
static void	 normal_round_pos(struct tablebase *, poscode, int, unsigned *, unsigned *);
static void	 mark_position(struct tablebase *, const struct position *, tb_entry);
static void	 count_wdl(const struct tablebase *);

/*
 * This structure is used to coordinate work between the threads.  The
 * members win, loss, round, and pc may only be modified while lock is
 * held.  round_barrier is used to synchronize the threads after one
 * round has finished.  win and loss contain the number of winning and
 * losing positions in the current round, round the current round and
 * pc the last chunk of the encoding space fetched for work.  The member
 * tb contains a pointer to the tablebase we currently work on.  It must
 * not be written asynchronously, lock doesn't need to be held to access
 * it.
 *
 * The workflow is as follows: Every thread has an internal round
 * counter.  When looking for work, the thread first locks lock and then
 * compares its own round counter to round.  If the values differ that
 * means that this thread is the first to do work on the new round and
 * initializes win, loss, and pc.  Then the thread takes one chunk of
 * work, increments pc appropriately and releases lock.  If the thread
 * was the first to do work in this round, it prints status information
 * from the previous round.  If no work is left to do, the thread
 * instead waits on round_barrier.  As a special case, if the thread
 * notices that it's the first to do work in the current round and the
 * loss counter stands at zero (meaning, no losses were found in the
 * previous round) then it leaves the state unchanged and terminates.
 */
struct gentb_state {
	pthread_mutex_t lock;

	/* members for which lock must be held */
	unsigned win, loss;
	unsigned round;
	poscode pc;

	/* members not protected by lock */
	pthread_barrier_t round_barrier;
	struct tablebase *tb;
};

/*
 * This function generates a complete tablebase and returns the
 * generated table base or NULL in case of error with errno containing
 * the reason for failure.  Progress information may be printed to
 * stderr in the process.  The threads argument indicates the number of
 * threads used to generate the tablebase.  The number of threads must
 * be positive and not larger than GENTB_MAX_THREADS.
 */
extern struct tablebase *
generate_tablebase(int threads)
{
	struct gentb_state gtbs;
	pthread_t pool[GENTB_MAX_THREADS];
	int i, j, error;

	if (threads <= 0) {
		errno = EINVAL;
		return (NULL);
	}

	if (threads > GENTB_MAX_THREADS)
		threads = GENTB_MAX_THREADS;

	memset(&gtbs, 0, sizeof gtbs);
	error = pthread_mutex_init(&gtbs.lock, NULL);
	if (error != 0) {
		errno = error;
		return (NULL);
	}

	error = pthread_barrier_init(&gtbs.round_barrier, NULL, threads);
	if (error != 0) {
		errno = error;
		return (NULL);
	}

	gtbs.tb = calloc(sizeof *gtbs.tb, 1);
	if (gtbs.tb == NULL)
		return (NULL);

	for (i = 0; i < threads; i++) {
		error = pthread_create(pool + i, NULL, gentb_worker, (void*)&gtbs);
		/* try to cleanup as much as possible */
		if (error != 0) {
			for (j = 0; j < i; j++)
				pthread_cancel(pool[j]);

			for (j = 0; j < i; j++)
				pthread_join(pool[j], NULL);

			free(gtbs.tb);
			errno = error;
			return (NULL);
		}
	}

	/* now all our threads are running, wait for them to finish */
	for (i = 0; i < threads; i++)
		pthread_join(pool[i], NULL);

	/* print final statistics */
	fprintf(stderr, "%9u  %9u\n", gtbs.win, gtbs.loss);

	/* this is fast enough to do synchronously */
	count_wdl(gtbs.tb);

	return (gtbs.tb);

/*
	initial_round(tb);
	round = 2;
	while (normal_round(tb, round))
		round++;
*/
}

/*
 * This function executes one thread to work on generating the
 * tablebase.  See the documentation for struct gentb_state for the
 * general process.
 */
static void *
gentb_worker(void *gtbs_arg)
{
	struct gentb_state *gtbs = gtbs_arg;
	poscode pc;
	unsigned round = 1, win = 0, loss = 0, print_stats;
	int error;

	for (;;) {
		print_stats = 0;

		error = pthread_mutex_lock(&gtbs->lock);
		assert(error == 0);

		/* are we the first thread to open a new round? */
		if (round > gtbs->round) {
			/*
			 * if we open a round, that means we either just
			 * waited on round_barrier or we are the first
			 * thread to run, so we can't have done any work
			 * before.
			 */
			assert(win == 0 && loss == 0);

			win = gtbs->win;
			loss = gtbs->loss;
			print_stats = 1;

			/* are we completely done? */
			if (loss == 0 && gtbs->round > 0) {
				error = pthread_mutex_unlock(&gtbs->lock);
				assert(error == 0);
				break;
			}

			++gtbs->round;
			gtbs->win = 0;
			gtbs->loss = 0;
			gtbs->pc.cohort = 0;
			gtbs->pc.lionpos = 0;
		} else {
			/* report results from previous chunk of work */
			gtbs->win += win;
			gtbs->loss += loss;
		}

		assert(round == gtbs->round);

		/* any work left to do? */
		if (gtbs->pc.cohort == COHORT_COUNT) {
			/* wait for more work */
			error = pthread_mutex_unlock(&gtbs->lock);
			assert(error == 0);
			error = pthread_barrier_wait(&gtbs->round_barrier);
			assert(error == 0 || error == PTHREAD_BARRIER_SERIAL_THREAD);
			round++;
			win = loss = 0;
			continue;
		}

		/* take work from gtbs */
		pc = gtbs->pc;
		gtbs->pc.lionpos++;
		if (gtbs->pc.lionpos == LIONPOS_COUNT) {
			gtbs->pc.lionpos = 0;
			gtbs->pc.cohort++;
		}

		error = pthread_mutex_unlock(&gtbs->lock);
		assert(error == 0);

		/*
		 * do costly IO after releasing the mutex to keep the
		 * duration we hold the mutex for as short as possible.
		 */
		if (print_stats) {
			if (round > 1)
				fprintf(stderr, "%9u  %9u\n", win, loss);

			fprintf(stderr, "Round %2u: ", round);
		}

		/* do the work we have taken */
		win = loss = 0;
		if (round == 1)
			initial_round_chunk(gtbs->tb, pc, &win, &loss);
		else
			normal_round_chunk(gtbs->tb, pc, &win, &loss, round);
	}

	return (NULL);
}

/*
 * In the initial round, every positions in the tablebase is evaluated.
 * Positions are categorized as:
 *
 *  - immediate wins (1) if gote_in_check() holds
 *  - checkmates (-1) if for each possible move sente_in_check() holds.
 *    This includes stalemates.
 *  - mate-in-one positions (2) if a checkmate can be reached.
 */
static void
initial_round_chunk(struct tablebase *tb, poscode pc, unsigned *win, unsigned *loss)
{
	unsigned size = cohort_size[pc.cohort].size;

	for (pc.map = 0; pc.map < size; pc.map++)
		for (pc.ownership = 0; pc.ownership < OWNERSHIP_COUNT; pc.ownership++)
			if (has_valid_ownership(pc))
				initial_round_pos(tb, pc, win, loss);
}

/*
 * For the initial round, evaluate one position indicated by pc and
 * store the result in tb.  Also increment win1 and loss1 if an
 * immediate win or checkmate is encountered.
 */
static void
initial_round_pos(struct tablebase *tb, poscode pc, unsigned *win1, unsigned *loss1)
{
	struct position p;
	struct unmove unmoves[MAX_UNMOVES];
	struct move moves[MAX_MOVES];
	size_t i, nmove, offset = position_offset(pc);
	int game_ended;

	decode_poscode(&p, pc);
	if (gote_in_check(&p)) {
		// tb->positions[offset] = 1;
		atomic_store_explicit(tb->positions + offset, 1, memory_order_relaxed);
		++*win1;
		return;
	}

	nmove = generate_moves(moves, &p);
	for (i = 0; i < nmove; i++) {
		struct position pp = p;
		game_ended = play_move(&pp, moves[i]);
		assert(!game_ended);

		if (!sente_in_check(&pp)) {
			/* position is not an immediate loss, can't judge it */
			return;
		}
	}

	/* all moves lead to a win for Gote */
	// tb->positions[offset] = -1;
	atomic_store_explicit(tb->positions + offset, -1, memory_order_relaxed);
	++*loss1;
	nmove = generate_unmoves(unmoves, &p);
	for (i = 0; i < nmove; i++) {
		struct position pp = p;

		undo_move(&pp, unmoves[i]);

		/*
		 * optimization: save encode_position() call for
		 * positions that are also mate in 1.
		 */
		if (!sente_in_check(&pp))
			mark_position(tb, &pp, 2);
	}
}

/*
 * In all but the first round we examine all positions that were marked
 * as wins for this round, do a retrograde analysis on them and then for
 * each position we find this way, we check if it's a losing position.
 * If it is, we mark the position as "lost" with the appropriate
 * distance to mate and every position reachable unmarked positions from
 * this as "won" with the appropriate distance to mate. If any positions
 * were marked in this last phase, nonzero is returned, zero otherwise.
 */
static void
normal_round_chunk(struct tablebase *tb, poscode pc, unsigned *win, unsigned *loss, unsigned round)
{
	unsigned size = cohort_size[pc.cohort].size;

	for (pc.map = 0; pc.map < size; pc.map++)
		for (pc.ownership = 0; pc.ownership < OWNERSHIP_COUNT; pc.ownership++)
			if (has_valid_ownership(pc))
				normal_round_pos(tb, pc, round, win, loss);
}

/*
 * Process one position in a normal round.
 */
static void
normal_round_pos(struct tablebase *tb, poscode pc, int round,
    unsigned *wins, unsigned *losses)
{
	struct position p;
	struct unmove unmoves[MAX_UNMOVES];
	size_t i, nunmove;

	if (tb->positions[position_offset(pc)] != round)
		return;

	++*wins;

	decode_poscode(&p, pc);
	nunmove = generate_unmoves(unmoves, &p);
	for (i = 0; i < nunmove; i++) {
		/* check if this is indeed a losing position */
		struct position ppmirror, pp = p;
		poscode pc;
		struct unmove ununmoves[MAX_UNMOVES];
		struct move moves[MAX_MOVES];
		tb_entry value;
		size_t j, nununmove, nmove, offset;

		undo_move(&pp, unmoves[i]);

		/* have we already analyzed this position? */
		encode_position(&pc, &pp);
		if (pc.lionpos >= LIONPOS_COUNT)
			continue;

		offset = position_offset(pc);
		if (tb->positions[offset] != 0)
			continue;

		/* make sure all moves are losing */
		nmove = generate_moves(moves, &pp);
		for (j = 0; j < nmove; j++) {
			struct position ppp = pp;

			play_move(&ppp, moves[j]);
			value = lookup_position(tb, &ppp);
			if (!is_win(value) || value > round)
				goto not_a_losing_position;
		}

		/* all moves are losing, mark positions as lost */
		value = atomic_exchange_explicit(tb->positions + offset, -round, memory_order_relaxed);
		//value = atomic_exchange(tb->positions + offset, -round);
		assert(value == 0 || value == -round);
		if (value == 0)
			++*losses;

		ppmirror = pp;
		if (position_mirror(&ppmirror)) {
			encode_position(&pc, &ppmirror);
			offset = position_offset(pc);
			value = atomic_exchange_explicit(tb->positions + offset, -round, memory_order_relaxed);
			//value = atomic_exchange(tb->positions + offset, -round);
			assert(value == 0 || value == -round);
			if (value == 0)
				++*losses;
		}

		/* mark all positions reachable from this one as won */
		nununmove = generate_unmoves(ununmoves, &pp);
		for (j = 0; j < nununmove; j++) {
			struct position ppp = pp;

			undo_move(&ppp, ununmoves[j]);

			if (!gote_in_check(&ppp))
				mark_position(tb, &ppp, round + 1);
		}

	not_a_losing_position:
		;
	}
}

/*
 * Mark position p and its mirrored variant as e in tb if it hasn't been
 * marked before.
 */
static void
mark_position(struct tablebase *tb, const struct position *p, tb_entry e)
{
	struct position pp = *p;
	poscode pc;
	size_t offset;

	encode_position(&pc, &pp);
	offset = position_offset(pc);
	assert(tb->positions[offset] >= 0);

	/*
	 * We only use this function to mark positions as won.  Thus,
	 * other threads might only attempt to concurrently mark this
	 * position as e and we don't have a test-and-set style race
	 * condition.
	 */
	if (tb->positions[offset] != 0)
		return;

	atomic_store_explicit(tb->positions + offset, e, memory_order_relaxed);

	if (!position_mirror(&pp))
		return;

	encode_position(&pc, &pp);
	offset = position_offset(pc);
	assert(tb->positions[offset] >= 0);

	if (tb->positions[offset] != 0)
		return;

	atomic_store_explicit(tb->positions + offset, e, memory_order_relaxed);
}

/*
 * Count how many positions are wins, draws, and losses and print the
 * figures to stderr.
 */
static void
count_wdl(const struct tablebase *tb)
{
	poscode pc;
	unsigned size, win = 0, draw = 0, loss = 0;
	tb_entry e;

	for (pc.cohort = 0; pc.cohort < COHORT_COUNT; pc.cohort++) {
		size = cohort_size[pc.cohort].size;
		for (pc.lionpos = 0; pc.lionpos < LIONPOS_COUNT; pc.lionpos++)
			for (pc.map = 0; pc.map < size; pc.map++)
				for (pc.ownership = 0; pc.ownership < OWNERSHIP_COUNT; pc.ownership++)
					if (has_valid_ownership(pc)) {
						e = tb->positions[position_offset(pc)];
						if (is_win(e))
							win++;
						else if (is_loss(e))
							loss++;
						else /* is_draw(e) */
							draw++;
					}
	}

	fprintf(stderr, "Total:    %9u  %9u  %9u\n", win, loss, draw);
}

/*
 * Write tb to file f.  It is assumed that f has been opened in binary
 * mode for writing and truncated.  This function returns 0 on success,
 * -1 on error with errno indicating the reason for failure.
 */
extern int
write_tablebase(FILE *f, const struct tablebase *tb)
{

	fwrite(tb->positions, sizeof tb->positions, 1, f);
	fflush(f);

	return (ferror(f) ? -1 : 0);
}
