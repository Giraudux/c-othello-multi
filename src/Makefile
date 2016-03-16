# Alexis Giraudet

.PHONY: clean

CFLAGS = -ansi -pedantic -Wall
CPPFLAGS = -D _REENTRANT
LDLIBS = -lpthread

all: othello-client othello-server

othello-client: othello-client.c

othello-server: othello-server.c

clean:
	-rm othello-client othello-server
