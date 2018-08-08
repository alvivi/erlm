SRC_DIR := src
SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
BIN_DIR := bin
BIN_FILES := $(patsubst $(SRC_DIR)/%.c,$(BIN_DIR)/%.o,$(SRC_FILES))
BIN_NOMAIN_FILES := $(filter-out $(BIN_DIR)/erlm.o, $(BIN_FILES))
TESTS_DIR := tests
TESTS_FILES := $(wildcard $(TESTS_DIR)/*.c)
TESTS_BIN_DIR := $(BIN_DIR)/$(TESTS_DIR)
TESTS_BIN_FILES := $(patsubst $(TESTS_DIR)/%.c,$(TESTS_BIN_DIR)/%,$(TESTS_FILES))

LIB_ARGPARSE_DIR := lib/argparse
LIB_ARGPARSE_SRC := $(LIB_ARGPARSE_DIR)/argparse.c
LIB_ARGPARSE_BIN := bin/argparse.o
LIB_DUKTAPE_DIR := lib/duktape
LIB_DUKTAPE_SRC := $(LIB_DUKTAPE_DIR)/duktape.c
LIB_DUKTAPE_BIN := bin/duktape.o
LIB_EI_DIR := $(shell erl -eval 'io:format("~s~n", [code:lib_dir(erl_interface, include)])' -s init stop -noshell)
LIB_EI := $(shell erl -eval 'io:format("~s", [code:lib_dir(erl_interface, lib)])' -s init stop -noshell)
LIB_TESTS_SRC := lib/acutest

INCS := -I$(SRC_DIR) -I$(LIB_EI_DIR) -I$(LIB_ARGPARSE_DIR) -I$(LIB_DUKTAPE_DIR)
LIBS := -lm -lerl_interface -lei -lpthread

CFLAGS ?= -std=c11 -Wall -Wextra

ifeq ($(DEBUG_BUILD),1)
	CFLAGS += -g -DDEBUG_BUILD
else
	CFLAGS += -O3
endif


.PHONY: all clean test

all: $(BIN_DIR)/erlm

clean:
	rm -Rf $(BIN_DIR)

test: $(TESTS_BIN_FILES)
	echo $^ | xargs -n 1 sh -c

$(BIN_DIR)/erlm: $(LIB_ARGPARSE_BIN) $(LIB_DUKTAPE_BIN) $(BIN_FILES)
	$(CC) $(CFLAGS) -o $@ $^ -L$(LIB_EI) $(LIBS)

$(LIB_ARGPARSE_BIN): $(LIB_ARGPARSE_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) -c -o $(LIB_ARGPARSE_BIN) $(LIB_ARGPARSE_SRC)

$(LIB_DUKTAPE_BIN): $(LIB_DUKTAPE_SRC) | $(BIN_DIR)
	$(CC) $(CFLAGS) -c -o $(LIB_DUKTAPE_BIN) $(LIB_DUKTAPE_SRC)

$(BIN_DIR)/%.o: $(SRC_DIR)/%.c | $(BIN_DIR)
	$(CC) $(CFLAGS) $(INCS) -c -o $@ $<

$(TESTS_BIN_DIR)/%: $(TESTS_DIR)/%.c $(LIB_ARGPARSE_BIN) $(LIB_DUKTAPE_BIN) $(BIN_NOMAIN_FILES) | $(TESTS_BIN_DIR)
	$(CC) -L$(LIB_EI) $(INCS) -I$(LIB_TESTS_SRC) -o $@ $^ $(LIBS)

$(BIN_DIR):
	mkdir -p $@

$(TESTS_BIN_DIR):
	mkdir -p $@
