CC=c99
CFLAGS=-O2

PLAYOBJ=moves.o movetable.o notation.o play.o validation.o
POSCODETESTOBJ=moves.o movetable.o notation.o validation.o poscode.o poscodetest.o

play: $(PLAYOBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o play $(PLAYOBJ)

poscodetest: $(POSCODETESTOBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o poscodetest $(POSCODETESTOBJ)

clean:
	rm -f *.o play poscodetest

.PHONY: clean
