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
