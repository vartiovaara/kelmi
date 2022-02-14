kelmi: *.c *.h
	$(CC) *.c magicmoves/magicmoves.c -o kelmi -Wall -Wextra -pedantic -O3 -march=native -mtune=native -g -std=c17

with-clang:
	clang *.c magicmoves/magicmoves.c -o kelmi -Wall -Wextra -pedantic -O2 -march=native -mtune=native -g -std=c17

clean:
	rm kelmi
