CC = cc

SRC = main.c algebraic.c attack.c bitboard.c board.c defs.h init.c lookup.c search.c magicmoves/magicmoves.c

CFLAGS = -Wall -Wextra -pedantic -O1 -mtune=generic -std=c17
NDFLAGS = -DNDEBUG -Wall -Wextra -pedantic -O3 -march=native -mtune=native -std=c17

all:
	cd src && \
	${CC} -o ../kelmi ${SRC} ${CFLAGS}

ndebug:
	cd src && \
	${CC} -o ../kelmi ${SRC} ${NDFLAGS}

clean:
	rm kelmi


#kelmi: *.c *.h
#	$(CC) *.c magicmoves/magicmoves.c -o kelmi -Wall -Wextra -pedantic -O3 -march=native -mtune=native -g -std=c17
#
#with-clang:
#	clang *.c magicmoves/magicmoves.c -o kelmi -Wall -Wextra -pedantic -O2 -march=native -mtune=native -g -std=c17
#
#clean:
#	rm kelmi
