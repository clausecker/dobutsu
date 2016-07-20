all: mapvalid countvalid checkposcode displaytest

postabs.c: gentabs
	./gentabs >postabs.c

gentabs: gentabs.o tables.o
	$(CC)  -o gentabs gentabs.o tables.o

mapvalid: mapvalid.o poscode.o postabs.o tables.o
	$(CC) -o mapvalid mapvalid.o poscode.o postabs.o tables.o

countvalid: countvalid.o poscode.o postabs.o tables.o
	$(CC) -o countvalid countvalid.o poscode.o postabs.o tables.o

checkposcode: checkposcode.o poscode.o tables.o display.o
	$(CC) -o checkposcode checkposcode.o poscode.o postabs.o tables.o display.o

displaytest: displaytest.o poscode.o tables.o display.o
	$(CC) -o displaytest displaytest.o poscode.o postabs.o tables.o display.o

clean:
	rm -f checkposcode countvalid gentabs mapvalid displaytest postabs.c *.o

.PHONY: clean

gentabs.o: dobutsu.h
tables.o: dobutsu.h
postabs.o: dobutsu.h
poscode.o: dobutsu.h
mapvalid.o: dobutsu.h
countvalid.o: dobutsu.h
checkposcode.o: dobutsu.h
display.o: dobutsu.h
displaytest.o: dobutsu.h
