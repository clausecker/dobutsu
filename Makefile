CC=c99
CFLAGS=-O2

PLAYOBJ=moves.o movetable.o notation.o play.o validation.o
POSCODETESTOBJ=moves.o movetable.o notation.o validation.o poscode.o poscodetest.o
TABLETESTOBJ=moves.o movetable.o poscode.o tabletest.o

all: play poscodetest tabletest

play: $(PLAYOBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o play $(PLAYOBJ)

poscodetest: $(POSCODETESTOBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o poscodetest $(POSCODETESTOBJ)

tabletest: $(TABLETESTOBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o tabletest $(TABLETESTOBJ)

clean:
	rm -f *.o play poscodetest tabletest

.PHONY: clean all
