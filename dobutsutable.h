#include <assert.h>

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
	LIONPOS_COUNT = 21,
	LIONPOS_TOTAL_COUNT = 41,
	OWNERSHIP_COUNT = 64,

	/* total positions in the table base */
	POSITION_COUNT = 255280704,

	MAX_PCALIAS = 16,
};

/*
 * cohort_table contains information for each cohort.  The following
 * information is stored:
 *
 *  - in pieces, how many of each kind of piece there are
 *  - in status, the chicken promotion bits
 *  - in sizes, how large the encoding space for each piece group
 *    ignoring the lions is.
 *
 * One byte of padding is added to make each entry eight bytes long.
 */
extern const struct cohort_info {
        unsigned char pieces[3]; /* 0: chicks, 1: giraffes, 2: elephants */
        unsigned char status; /* only promotion bits are set */
        unsigned char sizes[3];
        unsigned char padding; /* for alignment */
} cohort_info[COHORT_COUNT];

/*
 * cohort_size contains size information for each cohort.  The following
 * information is stored:
 *
 * - the offset of the beginning of data for that cohort in the tablebase
 * - the size of that cohort.
 *
 * this table is separate from cohort_info so that a record in each
 * table is 8 bytes long, allowing an indexed addressing mode to be used
 * on x86.
 */
extern const struct cohort_size {
	unsigned offset, size;
} cohort_size[COHORT_COUNT];

/*
 * This table contains a bitmap with a 1 for every combination of ownership
 * cohort that is valid.  This table is used by has_valid_ownweship().
 */
const unsigned long long valid_ownership_map[COHORT_COUNT];

/*
 * The tablebase struct contains a complete tablebase. It is essentially
 * just a huge array of positions.
 */
struct tablebase {
	signed char positions[POSITION_COUNT];
};

/*
 * A poscode (position code) is an encoded position directly suitable as
 * an index into the endgame tablebase.  A typedef is provided so we can
 * easily switch to an implementation where poscode is a numeric type.
 *
 * The poscode comprises the following pieces:
 *  - (0 -- 63) ownership is a bitmap indicating who owns which piece
 *  - (0 -- 62) cohort is a number indicating what pieces are on the
 *    board and if they are promoted
 *  - (0 -- 49) lionpos is a number indicating where the lions are
 *    placed.  All positions with lionpos 24 -- 49 are checkmates.
 *  - (0 -- 18899) map is a number indicating which pieces occupy what
 *    square.  The maximum value for this field depends on the value in
 *    cohort.
 *
 * Not all positions are stored in the tablebase: positions with both
 * lions adjacent or one lion already ascended aren't.  Neverthless,
 * even these positions are encodeable for simplicity.
 *
 * Before a position is encoded, it is normalized.  This means:
 *  - if it's Gote's move, the board is turned so a position with Sente
 *    to move obtains.
 *  - the board is flipped horizontally under certain conditions
 *  - pieces of the same kind are interchanged such that the _G piece
 *    always occupies a higher square than the _S piece where "in hand"
 *    is a higher square than all other squares.
 */
typedef struct {
	unsigned ownership;
	unsigned cohort;
	unsigned lionpos;
	unsigned map;
} poscode;

extern		int			encode_position(poscode*, const struct position*);
extern		int			decode_poscode(struct position*, poscode);
extern		int			position_mirror(struct position*);
static inline	size_t			position_offset(poscode);
static inline	int			has_valid_ownership(poscode);

/* inline functions */

/*
 * position_offset() returns the offset of pc in the tablebase.  It is
 * assumed that pc represents a valid position code that is in the table
 * base (i.e. with lionpos < LIONPOS_TOTAL).
 */
static inline size_t
position_offset(poscode pc)
{
	size_t index;

	assert(pc.lionpos < LIONPOS_COUNT);

	index = cohort_size[pc.cohort].offset;
	index += pc.lionpos * cohort_size[pc.cohort].size * OWNERSHIP_COUNT;
	index += pc.map * OWNERSHIP_COUNT + pc.ownership;

	return (index);
}

/*
 * To reduce the computational load, we only consider poscodes where for
 * each kind of piece, if both pieces are in hand, the _G piece is owned
 * by Gote only if the _S piece is owned by Gote, too (i.e. the allowed
 * ownership combinations are 11, 01, and 00).  This function checks if
 * this assertion holds and returns 1 if it does, 0 if it doesn't.
 */
static inline int
has_valid_ownership(poscode pc)
{
	return !!(valid_ownership_map[pc.cohort] & 1ULL << pc.ownership);
}
