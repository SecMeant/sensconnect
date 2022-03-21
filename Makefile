PHONY:=
CFLAGS:=-O0 -g3 -ggdb -Wall -Wextra

all: sensconnect

sensconnect: sensconnect.o
	$(CC) $(CFLAGS) $^ -o $@

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

PHONY += clean
clean:
	rm -f sensconnect *.o

.PHONY:=$(PHONY)
