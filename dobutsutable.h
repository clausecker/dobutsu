#include "tablebase.h"
#include "dobutsu.h"

/*
 * This header contains the low-level interface to the endgame tablebase
 * as well as prototypes for functions used to implement it.
 */

/*
 * the endgame tablebase is organized on four levels:
 *  1. by the cohort, that is, which pieces are on the board
 *  2. by the position of the lions on the board
 *  2. by the position of the other pieces on the board
 *  3. by piece ownership.
 *
 * For each cohort, there is a an array containing the distance-to-mate
 * (dtm) information for this cohort.  The index into this array is
 * is computed by multiplying lion position, map, and piece ownership
 * into one offset.  Separate arrays for each cohort are needed because
 * depending on how many pieces are on the board and which of the pieces
 * are distinguishable, the size of the cohort changes.
 *
 * The following constants describe how many of each encoding level
 * exist.
 */
enum {
	COHORT_COUNT = 63,
	LIONPOS_COUNT = 24,
	LIONPOS_TOTAL_COUNT = 50,
	OWNERSHIP_COUNT = 64,
};

/*
 * cohort_table contains information for each cohort.  The following
 * information is stored:
 *
 *  - the chicken promotion bits
 *  - how many of each kind of piece there are
 *  - how large the encoding space for each piece group is (ignoring
 *    the lions for which the space is always LIONPOS_TOTAL_COUNT).
 *
 * One byte of padding is added to make each entry eight bytes long.
 */
extern const struct cohort_info {
        unsigned char status; /* only promotion bits are set */
        unsigned char pieces[3]; /* 0: chicks, 1: giraffes, 2: elephants */
        unsigned char sizes[3];
        unsigned char padding;
} cohort_info[COHORT_COUNT];

