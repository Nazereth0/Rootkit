CC=gcc
CFLAGS=-c -Wall
KEYLOG_TARGETS=main.o keylogger.o find_event_file.o

all: keylog

keylog: $(KEYLOG_TARGETS)
	$(CC) $^ -o $@
