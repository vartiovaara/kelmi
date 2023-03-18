CC = cc

SRC = main.c algebraic.c bitboard.c board.c defs.h init.c uci.c lookup.c search.c movegen.c perft.c eval.c transposition.c random.c magicmoves/magicmoves.c

CFLAGS = -Wall -Wextra -pedantic -Og -ggdb3 -no-pie -mtune=generic -std=c17
NDCFLAGS = -DNDEBUG -Wall -Wextra -pedantic -O3 -flto -ftree-vectorize -fdevirtualize-at-ltrans -march=native -mtune=native -std=c17
GPROF_FLAGS = -DNDEBUG -Wall -Wextra -pedantic -pg -ggdb3 -O2 -march=native -mtune=native -std=c17

# TODO: Do testing with -funroll-loops
# TODO: Test -fdelete-null-pointer-checks -fno-stack-protector -frename-registers -fsplit-loops
# TODO: Test -fvariable-expansion-in-unroller

OUTPUTNAME = kelmi

all:
	cd src && \
	${CC} -o ../${OUTPUTNAME} ${SRC} ${CFLAGS}

ndebug:
	cd src && \
	${CC} -o ../${OUTPUTNAME} ${SRC} ${NDCFLAGS}

profile:
	cd src && \
	${CC} -o ../${OUTPUTNAME} ${SRC} -ggdb3 ${NDCFLAGS}

# Used for profiling with gprof
# gprof ./kelmi gmon.out
gprof:
	cd src && \
	&{CC} -o ../&{OUTPUTNAME} &{SRC} ${GPROF_FLAGS}

# https://cvw.cac.cornell.edu/vector/compilers_reports
#-fopt-info-vec-all
info:
	cd src && \
	${CC} -o ../${OUTPUTNAME} ${SRC} -ftree-vectorizer-verbose=2 -fopt-info-vec -fopt-info-vec-missed ${NDCFLAGS}


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
