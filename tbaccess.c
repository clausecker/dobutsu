/*-
 * Copyright (c) 2016--2017, 2021 Robert Clausecker. All rights reserved.
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
#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <lzma.h>

#include "dobutsutable.h"

static int read_xz_tablebase(FILE *f, struct tablebase *tb);

/*
 * Release all storage associated with tb.  The pointer to tb then
 * becomes invalid.
 */
extern void
free_tablebase(struct tablebase *tb)
{
	free(tb);
}

/*
 * Looks up a position in the table base, return its value.
 */
extern tb_entry
lookup_position(const struct tablebase *tb, const struct position *p)
{
	poscode pc, ppc;
	struct move moves[MAX_MOVES];
	struct position pp;
	size_t i, nmove;
	tb_entry e, worst = 1;
	int game_ends;

	/* checkmates aren't looked up */
	if (gote_moves(p) ? sente_in_check(p) : gote_in_check(p))
		return (1);

	encode_position(&pc, p);

	/* if the position is in the table base, look it up */
	if (ownership_map[pc.ownership] < OWNERSHIP_COUNT)
		return (tb->positions[position_offset(pc)]);

	/* otherwise, compute its value */
	nmove = generate_moves(moves, p);
	for (i = 0; i < nmove; i++) {
		pp = *p;
		game_ends = play_move(&pp, moves + i);
		assert(!game_ends);
		(void)game_ends;

		/* moving into check cannot be an improval */
		if (gote_moves(&pp) ? sente_in_check(&pp) : gote_in_check(&pp))
			continue;

		encode_position(&ppc, &pp);
		assert(ownership_map[ppc.ownership] < OWNERSHIP_COUNT);
		e = tb->positions[position_offset(ppc)];
		if (wdl_compare(e, worst) < 0)
			worst = e;
	}

	return (prev_dtm(worst));
}

/*
 * Read a tablebase from file f.  It is assumed that f has been opened
 * in binary mode for reading.  This function returns a pointer to the
 * newly loaded tablebase on success or NULL on error with errno
 * indicating the reason for failure.  Both uncompressed and compressed
 * table bases are supported, the code first tries to decompress the
 * table base, if it turns out to be uncompressed, another attempt is
 * made at reading an uncompressed tablebase.
 */
extern struct tablebase *
read_tablebase(FILE *f)
{
	struct tablebase *tb = malloc(sizeof *tb);
	off_t startpos;

	if (tb == NULL)
		return (NULL);

	if (startpos = ftello(f), startpos == -1)
		goto cleanup;

	switch (read_xz_tablebase(f, tb)) {
	case 0:
		return (tb);

	case 1:
		goto cleanup;

	case 2:
		if (fseeko(f, startpos, SEEK_SET) == -1)
			goto cleanup;

		if (fread((void*)tb->positions, sizeof tb->positions, 1, f) != 1)
			goto cleanup;

		return (tb);

	default:
		assert(0);
	}

cleanup:
	free(tb);
	return NULL;
}

/*
 * Read an xz compressed endgame tablebase.  Return 0 on success, 1 on
 * failure where the file could not possibly be an uncompressed
 * tablebase and 2 on failure where the file could be an uncompressed
 * tablebase.  In case of error, the tablebase contents are undefined.
 */
static int
read_xz_tablebase(FILE *f, struct tablebase *tb)
{
	lzma_stream strm = LZMA_STREAM_INIT;
	size_t count;
	int error;
	char inbuf[BUFSIZ];

	error = lzma_auto_decoder(&strm, UINT64_MAX, 0);
	switch (error) {
	case LZMA_OK:
		break;

	case LZMA_MEM_ERROR:
		return (1);

	case LZMA_OPTIONS_ERROR:
	case LZMA_PROG_ERROR:
	default:
		assert(0);
		abort();
	}

	strm.next_out = (uint8_t *)tb->positions;
	strm.avail_out = sizeof tb->positions;

	do {
		count = fread(inbuf, 1, sizeof inbuf, f);
		if (count == 0) {
			lzma_end(&strm);
			return (1);
		}

		strm.next_in = (uint8_t *)inbuf;
		strm.avail_in = sizeof inbuf;
		error = lzma_code(&strm, LZMA_RUN);
		if (error == LZMA_OK && strm.avail_in != 0) {
			assert(0);
			abort();
		}
	} while (error == LZMA_OK);

	lzma_end(&strm);

	switch (error) {
	case LZMA_STREAM_END:
		return (strm.avail_out == 0);

	case LZMA_FORMAT_ERROR:
		return (2);

	case LZMA_BUF_ERROR:
	case LZMA_PROG_ERROR:
	case LZMA_OPTIONS_ERROR:
	case LZMA_OK:
		assert(0);
		abort();

	default:
		return (1);
	}
}
