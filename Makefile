CC=c99
CFLAGS=-O2

PLAYOBJ=moves.o movetable.o notation.o play.o validation.o

play: $(PLAYOBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o play $(PLAYOBJ)

clean:
	rm -f *.o play

.PHONY: clean
