/*-
 * Copyright (c) 2016--2017 Robert Clausecker. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */
#include <assert.h>

#include "atomics.h"
#include "tablebase.h"
#include "dobutsu.h"

/*
 * This header contains the low-level interface to the endgame tablebase
 * as well as prototypes for functions used to implement it.
 */

/*
 * the endgame tablebase is organized on four levels:
 *  1. by piece ownership.
 *  2. by the cohort, that is, which pieces are on the board
 *  3. by the position of the lions on the board
 *  4. by the position of the other pieces on the board
 *
 * For each cohort, there is a an array containing the distance-to-mate
 * (dtm) information for this cohort.  The index into this array is
 * is computed by multiplying lion position, map, and piece ownership
 * into one offset.  Separate arrays for each cohort are needed because
 * depending on how many pieces are on the board and which of the pieces
 * are distinguishable, the size of the cohort changes.
 *
 * The following constants describe how many of each encoding level
 * exist.  If some values of an encoding level are possible but not
 * stored in the endgame tablebase, a corresponding _TOTAL macro is
 * present as well.
 */
enum {
	COHORT_COUNT = 63,
	LIONPOS_COUNT = 21,
	LIONPOS_TOTAL_COUNT = 41,
	OWNERSHIP_COUNT = 42,
	OWNERSHIP_TOTAL_COUNT = 64,


	/* total positions in the table base */
	POSITION_TOTAL_COUNT = 255280704,
	/* number of positions saved to disk (167527962) */
	POSITION_COUNT = POSITION_TOTAL_COUNT / OWNERSHIP_TOTAL_COUNT * OWNERSHIP_COUNT,

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
 * - the number of ways to arrange chicks, giraffes, and elephants in that
 *   cohort.
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
extern const unsigned long long valid_ownership_map[COHORT_COUNT];

/*
 * To save space, we only store positions in the table base where Sente
 * has no less pieces than Gote.  To facilate this, we permute the order
 * in which ownership is stored in the table such that all positions
 * with an equal or higher amount of pieces for Sente appear first.
 * This table contains a permutation of the ownership values such that
 * does values where Sente owns not less than three pieces are first.
 */
extern const unsigned char ownership_map[OWNERSHIP_TOTAL_COUNT];

/*
 * The tablebase struct contains a complete tablebase. It is essentially
 * just a huge array of positions.
 */
struct tablebase {
	atomic_schar positions[POSITION_COUNT];
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

extern		void			encode_position(poscode*, const struct position*);
extern		void			decode_poscode(struct position*, poscode);
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

	index = ownership_map[pc.ownership] * (POSITION_TOTAL_COUNT / OWNERSHIP_TOTAL_COUNT);
	index += cohort_size[pc.cohort].offset;
	index += cohort_size[pc.cohort].size * pc.lionpos;
	index += pc.map;

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
