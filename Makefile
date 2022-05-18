CC = cc

SRC = main.c algebraic.c bitboard.c board.c defs.h init.c uci.c lookup.c search.c movegen.c perft.c eval.c magicmoves/magicmoves.c

CFLAGS = -Wall -Wextra -pedantic -Og -g -mtune=generic -std=c17
NDCFLAGS = -DNDEBUG -Wall -Wextra -pedantic -Ofast -march=native -mtune=native -std=c17

OUTPUTNAME = kelmi

all:
	cd src && \
	${CC} -o ../${OUTPUTNAME} ${SRC} ${CFLAGS}

ndebug:
	cd src && \
	${CC} -o ../${OUTPUTNAME} ${SRC} ${NDCFLAGS}

clean:
	rm ${OUTPUTNAME}


#kelmi: *.c *.h
#	$(CC) *.c magicmoves/magicmoves.c -o kelmi -Wall -Wextra -pedantic -O3 -march=native -mtune=native -g -std=c17
#
#with-clang:
#	clang *.c magicmoves/magicmoves.c -o kelmi -Wall -Wextra -pedantic -O2 -march=native -mtune=native -g -std=c17
#
#clean:
#	rm kelmi
