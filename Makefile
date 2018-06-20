main: main.c png.o crc.o
	gcc -o main main.c png.o crc.o -Wall -lz
png.o: png.c png.h
	gcc -c -o png.o png.c -Wall
crc.o: crc.c crc.h
	gcc -c -o crc.o crc.c -Wall -lz
