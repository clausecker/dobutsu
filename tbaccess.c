#define _POSIX_C_SOURCE 200809L
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "xz/xz.h"
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
	struct xz_buf xzb;
	struct xz_dec *xzd;
	size_t count;
	int error;
	char inbuf[BUFSIZ];

	/* as this function is idempotent, call it just to be sure */
	xz_crc32_init();

	/* 4 MB is just the dictionary size we set in the Makefile */
	xzd = xz_dec_init(XZ_PREALLOC, 1LU << 22);
	if (xzd == NULL)
		return (1);

	xzb.in = (void*)inbuf;
	xzb.in_pos = xzb.in_size = sizeof inbuf;

	xzb.out = (void*)tb->positions;
	xzb.out_pos = 0;
	xzb.out_size = sizeof tb->positions;

	do {
		/*
		 * if error is XZ_OK, then we either need more input or
		 * more output space.  The latter can only happen if the
		 * file is not a tablebase file.
		 */
		if (xzb.in_pos != xzb.in_size)
			goto permanent_error;

		count = fread(inbuf, 1, sizeof inbuf, f);
		if (count == 0)
			goto permanent_error;

		xzb.in_pos = 0;
		xzb.in_size = count;
	} while (error = xz_dec_run(xzd, &xzb), error == XZ_OK);

	switch (error) {
	case XZ_STREAM_END:
		/* check if the file had the right size */
		if (xzb.out_pos != xzb.out_size)
			goto permanent_error;

		xz_dec_end(xzd);
		return (0);

	case XZ_UNSUPPORTED_CHECK:
	case XZ_MEM_ERROR:
	case XZ_MEMLIMIT_ERROR:
	case XZ_OPTIONS_ERROR:
	case XZ_DATA_ERROR:
		goto permanent_error;

	case XZ_FORMAT_ERROR:
		xz_dec_end(xzd);
		return (2);		

	/* these would indicate programming errors */
	case XZ_BUF_ERROR:
	case XZ_OK:
	default:
		assert(0);
		abort();
	}

permanent_error:
	xz_dec_end(xzd);
	return (1);
}
