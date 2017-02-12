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
#define _POSIX_C_SOURCE 200809L
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "tablebase.h"

/*
 * Generate the Dobutsu Shogi endgame tablebase, optionally in parallel.
 * The option -j nproc can be used to set the number of threads.
 */
extern int
main(int argc, char *argv[])
{
	struct tablebase *tb;
	FILE *tbfile;
	long threads = 1;
	int optchar;
	char *endptr;

	while(optchar = getopt(argc, argv, "j:"), optchar != -1)
		switch(optchar) {
		case 'j':
			threads = strtol(optarg, &endptr, 0);
			if (*optarg == '\0' || *endptr != '\0' || threads <= 0) {
				fprintf(stderr, "A positive number of threads is expected\n");
				return (EXIT_FAILURE);
			}

			if (threads > INT_MAX)
				threads = INT_MAX;

			break;

		case '?':
		default:
			goto usage;
		}


	if (argc - optind != 1) {
	usage:
		fprintf(stderr, "Usage: %s [-j nproc] dobutsu.tb\n", argv[0]);
		return (EXIT_FAILURE);
	}

	tbfile = fopen(argv[optind], "wb");
	if (tbfile == NULL) {
		perror("fopen");
		return (EXIT_FAILURE);
	}

	tb = generate_tablebase(threads);
	if (tb == NULL) {
		perror("generate_tablebase");
		return (EXIT_FAILURE);
	}

	if (write_tablebase(tbfile, tb)) {
		perror("write_tablebase");
		return (EXIT_FAILURE);
	}

	return (EXIT_SUCCESS);
}
