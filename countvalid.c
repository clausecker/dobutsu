#include <stdio.h>

#include "dobutsu.h"

/* count the number of valid positions */
extern int
main()
{
	pos_code i, invalid = 0, sente = 0, gote = 0, other;
	struct position p;

	for (i = 0; i < MAX_POS; i++) {
		if (i % 10000000 == 0)
			fprintf(stderr, "\r%10u", i);

		switch (decode_pos(&p, i)) {
		case POS_INVALID:
			invalid++;
			break;

		case POS_SENTE:
			sente++;
			break;

		case POS_GOTE:
			gote++;
			break;

		default:
			;
		}
	}

	fprintf(stderr, "\r%10u\n\n", i);

	fprintf(stderr, "invalid: %10u (%5.2f%%)\n", invalid, (100.0*invalid)/i);
	fprintf(stderr, "won:     %10u (%5.2f%%)\n", sente,   (100.0*sente  )/i);
	fprintf(stderr, "lost:    %10u (%5.2f%%)\n", gote,    (100.0*gote   )/i);
	other = i - invalid - sente - gote;
	fprintf(stderr, "other:   %10u (%5.2f%%)\n", other,   (100.0*other  )/i);

	return (0);
}
