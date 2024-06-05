CC := gcc
CFLAGS := -O -g -Wall -Werror -std=c99 -D_DEFAULT_SOURCE -D_GNU_SOURCE
LIBS := -lpthread -lncurses

SOURCES := client/client.c client/ui.c client/main.c src/pack.c src/unpack.c
OBJS := $(patsubst %.c,%.o,$(SOURCES))

all:  chat

test: $(TESTS)

chat: $(OBJS)
	$(CC) -o $@ $^ $(LIBS)

%.o: %.c
	$(CC) -c $< $(CFLAGS) -o $@

clean:
	rm -f src/*~ client/*~ $(OBJS)
	rm -f *~ chat submission.tar

submission: instant-messenger.tar

instant-messenger.tar: src/pack.c src/unpack.c
	tar cf instant-messenger.tar $^

.PHONY: all clean submission
