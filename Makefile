
all: srt9k

srt9k: Main.cpp CPU.cpp Disasm.cpp Disasm.h CPU.h Memory.h Common.h
	g++ -Wall -ansi -o srt9k -O3 -fomit-frame-pointer Main.cpp CPU.cpp Disasm.cpp `sdl-config --cflags --libs` -lSDL_ttf

image: simple.bin simple.dump.txt

clean:
	rm -rf srt9k simple.o simple.elf simple.dump.txt simple.bin *.o

simple.bin: simple.elf
	mips-elf-objcopy -O binary simple.elf simple.bin
	
simple.dump.txt: simple.elf
	mips-elf-objdump -xD simple.elf > simple.dump.txt
	
simple.elf: simple.o linkscript.ld
	mips-elf-ld -T linkscript.ld -o simple.elf simple.o

simple.o: simple.S
	mips-elf-as -mips1 simple.S -o simple.o
	
