all: erg1

erg1: main.c
	gcc -o erg1 main.c -lpthread

clean:
	rm erg1

run:
	./erg1 input.txt 3 3

