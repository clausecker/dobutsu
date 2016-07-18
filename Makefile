all: mapvalid countvalid checkposcode displaytest

postab.c: gentabs
	./gentabs >postab.c

gentabs: gentabs.o moves.o
	$(CC)  -o gentabs gentabs.o moves.o

mapvalid: mapvalid.o poscode.o moves.o
	$(CC) -o mapvalid mapvalid.o poscode.o moves.o

countvalid: countvalid.o poscode.o moves.o
	$(CC) -o countvalid countvalid.o poscode.o moves.o

checkposcode: checkposcode.o poscode.o moves.o display.o
	$(CC) -o checkposcode checkposcode.o poscode.o moves.o display.o

displaytest: displaytest.o poscode.o moves.o display.o
	$(CC) -o displaytest displaytest.o poscode.o moves.o display.o

clean:
	rm -f checkposcode countvalid gentabs mapvalid postab.c *.o

.PHONY: clean

gentabs.o: dobutsu.h
moves.o: dobutsu.h
poscode.o: postab.c dobutsu.h
mapvalid.o: dobutsu.h
countvalid.o: dobutsu.h
checkposcode.o: dobutsu.h
display.o: dobutsu.h
displaytest.o: dobutsu.h
