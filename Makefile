all: mapvalid countvalid mapcompact

postab.c: gentabs
	./gentabs >postab.c

gentabs: gentabs.o moves.o
	$(CC)  -o gentabs gentabs.o moves.o

mapvalid: mapvalid.o poscode.o moves.o
	$(CC) -o mapvalid mapvalid.o poscode.o moves.o

mapcompact: mapcompact.o poscode.o moves.o
	$(CC) -o mapcompact mapcompact.o poscode.o moves.o

countvalid: countvalid.o poscode.o moves.o
	$(CC) -o countvalid countvalid.o poscode.o moves.o

gentabs.o: dobutsu.h
moves.o: dobutsu.h
poscode.o: postab.c dobutsu.h
mapvalid.o: dobutsu.h
mapcompact.o: dobutsu.h
countvalid.o: dobutsu.h
