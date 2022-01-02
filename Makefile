kelmi: *.c *.h
	$(CC) *.c -o kelmi -Wall -Wextra -pedantic -O2 -march=native -mtune=native -g -std=c17

with-clang:
	clang *.c -o kelmi -Wall -Wextra -pedantic -O2 -march=native -mtune=native -g -std=c17

clean:
	rm kelmi
