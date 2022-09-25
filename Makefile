# Build bcc fdisk

std=-std=c11

debug: fdisk-bcc-debug

fdisk-bcc-debug: bin/main.o
	gcc $(std) -O0 -o bin/fdisk-bcc bin/main.o -g -W -Wall -pedantic

bin/main.o: main.c bin
	gcc $(std) -O0 -o bin/main.o main.c -c -W -Wall -pedantic

bin:
	mkdir bin
	sudo dd if=/dev/sda of=bin/mbr.bin bs=512 count=1

clean:
	rm -rf bin