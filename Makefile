kelmi: *.c *.h
	$(CC) *.c -o kelmi -Wall -Wextra -pedantic -O2 -g -std=c17

with-clang:
	clang *.c -o kelmi -Wall -Wextra -pedantic -O2 -g -std=c17

clean:
	rm kelmi
