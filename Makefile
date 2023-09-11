CC = cc
#CC=clang

#SRC = src/main.c src/algebraic.c src/bitboard.c src/board.c src/init.c src/uci.c src/lookup.c src/search.c src/movegen.c src/movefactory.c src/perft.c src/eval.c src/transposition.c src/random.c
SRC = src/main.c src/algebraic.c src/board.c src/init.c src/lookup.c src/movegen.c src/perft.c src/random.c

#DEBUG_CFLAGS = -Wall -Wextra -pedantic -Og -ggdb3 -no-pie -mtune=generic -std=c17
#CFLAGS = -Wall -Wextra -pedantic -Og -ggdb3 -no-pie -fstack-check -fstack-protector-all -fsanitize=address -fno-omit-frame-pointer -fsanitize=undefined -fsanitize=null -fsanitize=bounds-strict -std=c17
#CFLAGS = -Wall -Wextra -pedantic -Og -ggdb3 -no-pie -std=c17
#CFLAGS = -DNDEBUG -Wall -Wextra -pedantic -O3 -flto -ftree-vectorize -march=native -mtune=native -std=c17
CFLAGS = -DNDEBUG -Wall -Wextra -pedantic -O3 -flto -march=native -mtune=native -std=c17
#CFLAGS = -DNDEBUG -Wall -Wextra -pedantic -pg -ggdb3 -O2 -fno-inline -march=native -mtune=native -std=c17
#CFLAGS = -DNDEBUG -Wall -Wextra -pedantic -ggdb3 -O3 -no-pie -fno-inline -march=native -mtune=native -std=c17
#GPROF_CFLAGS = -DNDEBUG -Wall -Wextra -pedantic -pg -ggdb3 -O2 -fno-inline -march=native -mtune=native -std=c17

#LDFLAGS = -flto
#LDFLAGS = -g
#LDFLAGS = -pg

TARGET = kelmi

OBJECTS=$(SRC:.c=.o)
#OBJECTS = $(patsubst %.c,%.o,$(SRC))


all: $(TARGET)


ndebug:
	cd src && \
	${CC} -o ../${OUTPUTNAME} ${SRC} ${NDCFLAGS}


clean:
	rm $(TARGET) $(OBJECTS)


#$(OBJECTS):
#	$(CC) $(CFLAGS) -c $($^:.c=.o) -o $@

$(TARGET): $(OBJECTS)
	$(CC) -o $@ $(OBJECTS) $(CFLAGS) $(LDFLAGS)



#kelmi: *.c *.h
#	$(CC) *.c magicmoves/magicmoves.c -o kelmi -Wall -Wextra -pedantic -O3 -march=native -mtune=native -g -std=c17
#
#with-clang:
#	clang *.c magicmoves/magicmoves.c -o kelmi -Wall -Wextra -pedantic -O2 -march=native -mtune=native -g -std=c17
#
#clean:
#	rm kelmi
