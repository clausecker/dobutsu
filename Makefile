CC=c99
CFLAGS=-O3 -DNDEBUG

GENTBOBJ=gentb.o tbaccess.o tbgenerate.o moves.o unmoves.o poscode.o
VALIDATETBOBJ=validatetb.o tbaccess.o tbvalidate.o moves.o poscode.o notation.o validation.o
DOBUTSUOBJ=dobutsu.o tbaccess.o moves.o unmoves.o poscode.o notation.o validation.o ai.o position.o

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
