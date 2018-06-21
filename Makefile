test: test.c png.o crc.o filter.o
	gcc -g -o test test.c png.o crc.o filter.o -Wall -lz
png.o: png.c png.h filter.o
	gcc -g -c -o png.o png.c -Wall
filter.o: filter.c
	gcc -g -c -o filter.o filter.c -Wall
crc.o: crc.c crc.h
	gcc -g -c -o crc.o crc.c -Wall -lz

lib: 
	gcc -c -fpic -o png.o png.c
	gcc -c -fpic -o crc.o crc.c
	gcc -c -fpic -o filter.o filter.c
	gcc -shared -o libsimplepng.so png.o filter.o crc.o
