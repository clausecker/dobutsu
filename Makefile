all: countvalid checkposcode displaytest gendb

postabs.c: gentabs
	./gentabs >postabs.c

gentabs: gentabs.o tables.o
	$(CC)  -o gentabs gentabs.o tables.o

gendb: gendb.o poscode.o postabs.o tables.o moves.o
	$(CC) -o gendb gendb.o poscode.o postabs.o tables.o moves.o

countvalid: countvalid.o poscode.o postabs.o tables.o moves.o display.o
	$(CC) -o countvalid countvalid.o poscode.o postabs.o tables.o moves.o display.o

checkposcode: checkposcode.o poscode.o postabs.o tables.o display.o
	$(CC) -o checkposcode checkposcode.o poscode.o postabs.o tables.o display.o

displaytest: displaytest.o poscode.o postabs.o tables.o display.o
	$(CC) -o displaytest displaytest.o poscode.o postabs.o tables.o display.o

clean:
	rm -f checkposcode countvalid gendb gentabs mapvalid displaytest postabs.c *.o

.PHONY: clean

gendb.o: dobutsu.h
gentabs.o: dobutsu.h
tables.o: dobutsu.h
postabs.o: dobutsu.h
poscode.o: dobutsu.h
moves.o: dobutsu.h
countvalid.o: dobutsu.h
checkposcode.o: dobutsu.h
display.o: dobutsu.h
displaytest.o: dobutsu.h
