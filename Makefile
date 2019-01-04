CFLAGS = -Wall -g
CC     = gcc $(CFLAGS)

all : bl-server bl-client

bl-server : bl-server.o server.o util.o blather.h
	$(CC) -o $@ $^

bl-client : bl-client.o simpio.o util.o blather.h -lpthread
	$(CC) -o $@ $^

simpio-demo : simpio-demo.o simpio.o -lpthread
	$(CC) -o $@ $^

shell-tests : shell_tests.sh shell_tests_data.sh cat-sig.sh clean-tests
	chmod u+rx shell_tests.sh shell_tests_data.sh normalize.awk filter-semopen-bug.awk cat-sig.sh
	./shell_tests.sh

server.o : server.c blather.h
	$(CC) -c $<

simpio.o : simpio.c blather.h
	$(CC) -c $<

simpio-demo.o : simpio-demo.c blather.h
	$(CC) -c $<

util.o : util.c blather.h
	$(CC) -c $<

clean:
	rm -f *.o *.fifo bl-server bl-client simpio-demo

clean-tests :
	rm -f test-*.log test-*.out test-*.expect test-*.diff test-*.valgrindout
