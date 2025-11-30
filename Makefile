all: build

build:
	gcc main.c -o chip -lX11

clean:
	rm -f chip
