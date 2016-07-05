#include <stdio.h>

#include "dobutsu.h"

/* count the number of valid positions */
extern int
main()
{
	pos_code i, v;
	struct position p;

	for (i = 0, v = 0; i < MAX_POS; i++) {
		if (i % 10000000 == 0)
			fprintf(stderr, "\r%10u/%10u = %5.2f%%", v, i, (100.0*v)/i);

		v += decode_pos(&p, i) == 0;
	}

	fprintf(stderr, "\r%10u/%10u = %5.2f%%\n", v, i, (100.0*v)/i);
	return (0);
}
