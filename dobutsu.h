/* fields on the board:
 *
 * B A 9
 * 8 7 6
 * 5 4 3
 * 2 1 0
 *
 * everything else means piece is on hand.
 */

/* an encoded game position */
typedef unsigned pos_code;
typedef unsigned short board;

#define MAX_POS (pos_code)(24 * 3 * 3 * 10 * 11 * 11 * 11 * 11 * 11 * 11U)

/* an unpacked game position */
struct position {
	unsigned char l, L, c, C, e, E, g, G;
	unsigned char op;
};

/* for struct position.op */
enum {
	co = 1 << 0,	/* owner 1st chick */
	Co = 1 << 1,	/* owner 2nd chick */
	cp = 1 << 2,	/* promotion 1st chick */
	Cp = 1 << 3,	/* promotion 2nd chick */
	eo = 1 << 4,	/* owner 1st elephant */
	Eo = 1 << 5,	/* owner 2nd elephant */
	go = 1 << 6,	/* owner 1st giraffe */
	Go = 1 << 7	/* owner 2nd giraffe */
};

/* from poscode.c */
extern pos_code encode_pos(const struct position*);
extern int decode_pos(struct position*, pos_code);
extern pos_code pos_is_legal(const struct position*);

/* from moves.c */
/* first 12 are sente's moves, the second 12 are gote's moves */
extern const board Rmoves[12], rmoves[12], Llmoves[12], Ggmoves[12], Eemoves[12], Cmoves[12], cmoves[12];

/* return values for encode_pos */
enum {
	POS_OK      = 0,		/* used internally, not actually returned */
	POS_INVALID = 0xffffffff,	/* invalid position */
	POS_SENTE   = 0xfffffffe,	/* gote lion is mated or sente lion is on second row */
	POS_GOTE    = 0xfffffffd,	/* sente lion is mated or gote lion is on third row */
};

extern void flip_pos(struct position*);
