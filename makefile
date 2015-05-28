all: archer

archer: main.o
	gcc main.o -o bin/archer -lpthread

debug: main.o
	gcc -g main.o -o bin/archer

main.o: src/main.c
	gcc -c src/main.c

clean:
	rm -rf *.o bin/archer
