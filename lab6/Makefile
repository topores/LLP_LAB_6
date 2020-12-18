all: clean main

main: main.c
	gcc -c main.c bmp.c
	gcc -o main  -pedantic -ansi -Waddress -Wfatal-errors -Wnonnull main.o bmp.o -lm

clean:
	rm -f main *.o