all:
	gcc main.c -o matrix -Wall -pthread

clean:
	rm -f matrix
