NAME=riscosbasic

.PHONY: clean all debug install

CFLAGS=-Wall

PREFIX=/usr/local

all: riscosbasic

riscosbasic: riscosbasic.c keywords

keywords: keywords.c
	@touch keywords

clean:
	rm -f riscosbasic keywords *.o

install: all
	install $(NAME) $(PREFIX)/bin
