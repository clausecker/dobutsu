CC=c99
CFLAGS=-O2

PLAYOBJ=moves.o unmoves.o notation.o play.o validation.o tbaccess.o poscode.o
POSCODETESTOBJ=moves.o unmoves.o notation.o validation.o poscode.o poscodetest.o
GENDBOBJ=gendb.o tbaccess.o tbgenerate.o moves.o unmoves.o poscode.o
VALIDATEDBOBJ=validatedb.o tbaccess.o tbvalidate.o moves.o poscode.o notation.o validation.o

all: play poscodetest gendb validatedb

play: $(PLAYOBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o play $(PLAYOBJ)

poscodetest: $(POSCODETESTOBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o poscodetest $(POSCODETESTOBJ)

gendb: $(GENDBOBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o gendb $(GENDBOBJ)

validatedb: $(VALIDATEDBOBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o validatedb $(VALIDATEDBOBJ)

clean:
	rm -f *.o play poscodetest gendb validatedb

.PHONY: clean all
