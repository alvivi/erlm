
ERL_EI_INCLUDE = $(shell erl -eval 'io:format("~s~n", [code:lib_dir(erl_interface, include)])' -s init stop -noshell)
ERL_EI_LIB = $(shell erl -eval 'io:format("~s", [code:lib_dir(erl_interface, lib)])' -s init stop -noshell)

CFLAGS = -std=c11 -O3 -Wall -Wextra -fPIC 

all: bin/erlm

bin/erlm: bin src/erlm.c bin/argparse.o bin/duktape.o bin/timers.o bin/port.o
	$(CC) $(CFLAGS) -L$(ERL_EI_LIB) -lm -lerl_interface -lei -lpthread -I$(ERL_EI_INCLUDE) -Ilib/duktape -Ilib/argparse src/erlm.c bin/port.o bin/timers.o bin/duktape.o bin/argparse.o -o bin/erlm

bin/duktape.o: lib/duktape/duktape.c
	$(CC) $(CFLAGS) -o bin/duktape.o -c lib/duktape/duktape.c

bin/argparse.o: lib/argparse/argparse.c
	$(CC) $(CFLAGS) -o bin/argparse.o -c lib/argparse/argparse.c

bin/timers.o: src/timers.c
	$(CC) $(CFLAGS) -Ilib/duktape -o bin/timers.o -c src/timers.c

bin/port.o: src/port.c
	$(CC) $(CFLAGS) -o bin/port.o -c src/port.c

bin:
	mkdir -p bin

clean:
	rm -rf bin
