test: test.c png.o crc.o filter.o transform.o
	gcc -g -o test test.c png.o crc.o filter.o transform.o -Wall -lz
png.o: png.c png.h filter.o
	gcc -g -c -o png.o png.c -Wall
filter.o: filter.c
	gcc -g -c -o filter.o filter.c -Wall
transform.o: transform.c
	gcc -g -c -o transform.o transform.c -Wall
crc.o: crc.c
	gcc -g -c -o crc.o crc.c -Wall -lz

lib:
	gcc -c -o png.o png.c
	gcc -c -o crc.o crc.c
	gcc -c -o filter.o filter.c
	gcc -c -o transform.o transform.c
	ar cr libsimplepng.a png.o filter.o crc.o

lib_so: 
	gcc -c -fpic -o png.o png.c
	gcc -c -fpic -o crc.o crc.c
	gcc -c -fpic -o filter.o filter.c
	gcc -c -fpic -o transform.o transform.c
	gcc -shared -o libsimplepng.so png.o filter.o crc.o
