kelmi: *.c *.h
	$(CC) main.c -o kelmi -Wall -Wextra -pedantic -O2 -g -std=c17

clean:
	rm paskasankko
