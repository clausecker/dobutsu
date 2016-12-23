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

	tbfile = fopen(argv[1], "rb");
	if (tbfile == NULL) {
		perror("fopen");
		return (EXIT_FAILURE);
	}

	tb = read_tablebase(tbfile);
	if (tb == NULL) {
		perror("read_tablebase");
		return (EXIT_FAILURE);
	}

	if (validate_tablebase(tb))
		return (EXIT_SUCCESS);
	else
		return (EXIT_FAILURE);
}
