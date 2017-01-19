CC=c99
CFLAGS=-O3 -DNDEBUG

GENTBOBJ=gentb.o tbgenerate.o poscode.o unmoves.o moves.o
VALIDATETBOBJ=$(XZOBJ) validatetb.o tbvalidate.o tbaccess.o notation.o poscode.o validation.o moves.o
DOBUTSUOBJ=$(XZOBJ) dobutsu.o position.o ai.o notation.o tbaccess.o validation.o poscode.o moves.o
XZOBJ=xz/xz_crc32.o xz/xz_dec_lzma2.o xz/xz_dec_stream.o

all: gentb validatetb dobutsu

gentb: $(GENTBOBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o gentb $(GENTBOBJ) $(LDLIBS) -lpthread

validatetb: $(VALIDATETBOBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o validatetb $(VALIDATETBOBJ) $(LDLIBS)

dobutsu: $(DOBUTSUOBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o dobutsu $(DOBUTSUOBJ) $(LDLIBS) -lm

dobutsu.tb.xz: dobutsu.tb
	rm -f dobutsu.tb.xz
	# dictionary size must be harmonized with code in tbaccess.c
	xz -k --lzma2=preset=4e,dict=4M,lc=1,lp=3,pb=4 -C crc32 dobutsu.tb

dobutsu.tb: gentb
	./gentb dobutsu.tb

clean:
	rm -f *.o xz/*.o gentb validatetb dobutsu

.PHONY: clean all

.POSIX:
