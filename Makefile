test: test.c png.o crc.o
	gcc -g -o test test.c png.o crc.o -Wall -lz
png.o: png.c png.h
	gcc -g -c -o png.o png.c -Wall
crc.o: crc.c crc.h
	gcc -g -c -o crc.o crc.c -Wall -lz
