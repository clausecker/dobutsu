#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>

#include "dobutsu.h"

/* create a prototype for the database: every valid position gets a 1,
 * every invalid position stays at 0.
 */

extern int
main(int argc, char *argv[])
{
	pos_code i, v;
	struct position p;
	int fd, ret;
	unsigned char *map;

	if (argc != 2) {
		fprintf(stderr, "Usage: %s file.db\n", argv[0]);
		return (1);
	}

	fd = open(argv[1], O_RDWR | O_CREAT, 0777);
	if (fd == -1) {
		perror("Cannot open database");
		return (1);
	}

	if (ftruncate(fd, MAX_POS) == -1) {
		perror("Cannot truncate database");
		close(fd);
		return (1);
	}

	map = mmap(NULL, MAX_POS, PROT_WRITE, MAP_SHARED, fd, 0);
	if (map == MAP_FAILED) {
		perror("Cannot map file");
		close(fd);
		return (1);
	}

	for (i = 0, v = 0; i < MAX_POS; i++) {
		if (i % 10000000 == 0)
			fprintf(stderr, "\r%10u/%10u = %5.2f%%", v, i, (100.0*v)/i);

		v += (map[i] = decode_pos(&p, i) == 0);
	}

	fprintf(stderr, "\r%10u/%10u = %5.2f%%\n", v, i, (100.0*v)/i);

	munmap(map, MAX_POS);
	close(fd);

	return (0);
}
