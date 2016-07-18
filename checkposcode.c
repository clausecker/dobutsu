#include <stdio.h>
#include <stdlib.h>

#include "dobutsu.h"

/* checkposcode -- make sure that encode_pos is the reverse of decode_pos */

extern int
main()
{
	pos_code pc, new_pc;
	int status = EXIT_SUCCESS;
	struct position pos;

	for (pc = 0; pc < MAX_POS; pc++) {
		if (decode_pos(&pos, pc) != POS_OK)
			continue;

		new_pc = encode_pos(&pos);
		if (new_pc != pc) {
			if (new_pc >= MAX_POS)
				printf("%10u -> (%10u) ", pc, new_pc);
			else
				printf("%10u ->  %10u  ", pc, new_pc);

			show_pos(&pos);
			printf(" -> ");
			if (decode_pos(&pos, new_pc) != 0)
				printf("(invalid)\n");
			else {
				show_pos(&pos);
				printf("\n");
			}

			status = EXIT_FAILURE;
		}
	}

	return (status);
}
