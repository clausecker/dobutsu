CC=c99
CFLAGS=-O2

GENTBOBJ=gentb.o tbaccess.o tbgenerate.o moves.o unmoves.o poscode.o
VALIDATEDBOBJ=validatedb.o tbaccess.o tbvalidate.o moves.o poscode.o notation.o validation.o

all: gentb validatedb

gentb: $(GENTBOBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o gentb $(GENTBOBJ) $(LDLIBS)

validatedb: $(VALIDATEDBOBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o validatedb $(VALIDATEDBOBJ) $(LDLIBS)

dobutsu.tb.xz: dobutsu.tb
	xz -k --lzma2=preset=4e,lc=1,lp=3,pb=4 -C crc32 dobutsu.tb

dobutsu.tb: gentb
	./gentb dobutsu.tb

clean:
	rm -f *.o gentb validatedb

.PHONY: clean all
