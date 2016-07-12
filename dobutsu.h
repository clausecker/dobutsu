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

#define MAX_POS (pos_code)(256 * 24 * 56 * 56 * 56U)

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

/* for invariants in poscode.c and gentabs.c */
enum {
	CINVARIANT = 1 << 0,
	EINVARIANT = 1 << 1,
	GINVARIANT = 1 << 2
};

/* from poscode.c */
extern pos_code encode_pos(const struct position*);
extern int decode_pos(struct position*, pos_code);
extern pos_code pos_is_legal(const struct position*);

/* from moves.c */
extern const board Rmoves[13], rmoves[13], Llmoves[13], Ggmoves[13], Eemoves[13], Cmoves[13], cmoves[13];

/* return values for encode_pos */
enum {
	POS_OK      = 0,  /* used internally, not actually returned */
	POS_INVALID = -1, /* invalid position */
	POS_SENTE   = -2, /* gote lion is mated or sente lion is on second row */
	POS_GOTE    = -3, /* sente lion is mated or gote lion is on third row */
};

extern void flip_pos(struct position*);
