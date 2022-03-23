CFLAGS:=-O1 -g3 -ggdb -Wall -Wextra -std=c17 -fno-stack-protector -fno-omit-frame-pointer

PHONY:=

all: sensconnect

sensconnect: sensconnect.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

PHONY += clean
clean:
	rm -f sensconnect *.o

.PHONY:=$(PHONY)
