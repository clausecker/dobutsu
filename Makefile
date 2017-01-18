CC=c99
CFLAGS=-O3 -DNDEBUG

GENTBOBJ=gentb.o tbgenerate.o tbaccess.o poscode.o unmoves.o moves.o
VALIDATETBOBJ=validatetb.o tbvalidate.o tbaccess.o notation.o poscode.o validation.o moves.o
DOBUTSUOBJ=dobutsu.o position.o ai.o notation.o tbaccess.o validation.o poscode.o moves.o

all: gentb validatetb dobutsu

gentb: $(GENTBOBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o gentb $(GENTBOBJ) $(LDLIBS) -lpthread

validatetb: $(VALIDATETBOBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o validatetb $(VALIDATETBOBJ) $(LDLIBS)

dobutsu: $(DOBUTSUOBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o dobutsu $(DOBUTSUOBJ) $(LDLIBS) -lm

dobutsu.tb.xz: dobutsu.tb
	rm -f dobutsu.tb.xz
	xz -k --lzma2=preset=4e,lc=1,lp=3,pb=4 -C crc32 dobutsu.tb

dobutsu.tb: gentb
	./gentb dobutsu.tb

clean:
	rm -f *.o gentb validatetb dobutsu

.PHONY: clean all

.POSIX:
