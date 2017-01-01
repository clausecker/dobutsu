#include <stdlib.h>
#include <stdio.h>

#include "tablebase.h"

extern int
main(int argc, char *argv[])
{
	struct tablebase *tb;
	FILE *tbfile;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s game.db\n", argv[0]);
		return (EXIT_FAILURE);
	}

	tbfile = fopen(argv[1], "wb");
	if (tbfile == NULL) {
		perror("fopen");
		return (EXIT_FAILURE);
	}

	tb = generate_tablebase();
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
