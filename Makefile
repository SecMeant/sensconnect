CFLAGS:=-O1 -Wall -Wextra -std=c17 -fno-stack-protector -fno-omit-frame-pointer -no-pie -mno-sse

PHONY:=

all: sensconnect

sensconnect: sensconnect.o
	$(CC) $(CFLAGS) $^ -o $@
	strip $@

%.o: %.c Makefile
	$(CC) $(CFLAGS) -c $< -o $@

PHONY += clean
clean:
	rm -f sensconnect *.o

.PHONY:=$(PHONY)
