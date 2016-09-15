default: dobutsu

all: countvalid checkposcode displaytest gendb dobutsu

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

displaytest: displaytest.o poscode.o postabs.o tables.o display.o moves.o notation.o
	$(CC) -o displaytest displaytest.o poscode.o postabs.o tables.o display.o moves.o notation.o

dobutsu: dobutsu.o poscode.o postabs.o tables.o display.o moves.o notation.o gamedb.o
	$(CC) -o dobutsu dobutsu.o poscode.o postabs.o tables.o display.o moves.o notation.o gamedb.o

clean:
	rm -f checkposcode countvalid gendb gentabs mapvalid displaytest dobutsu postabs.c *.o

game.db: gendb
	./gendb game.db

fetch-gamedb:
	rm -f game.db
	wget -O - http://fuz.su/~fuz/files/game.db.xz | unxz -c >game.db
	[ "`cksum game.db`" = "516306943 1078984704 game.db" ]
	chmod a-w game.db

.PHONY: all clean fetch-gamedb

.POSIX:

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
notation.o: dobutsu.h
gamedb.o: dobutsu.h
dobutsu.o: dobutsu.h
