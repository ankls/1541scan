SOURCES = ../src/errors.c \
          ../src/disk.c \
          ../src/channel.c \
		  ../src/kernal_io.c \
		  ../src/opencbm_io.c \
          ../src/dos_io.c \
          ../src/display_aux.c \
		  ../src/keyboard.c \
          ../src/1541scan.c

TARGET = 1541scan

debug:
	mkdir -p build
	cd build && cl65 -Oirs -I/usr/lib64/cc65/include -I../src -L/usr/lib64/cc65  -vm -T -l 1541scan.asm -v $(SOURCES) -Wl "--mapfile,${TARGET}.map" -Wl "--dbgfile,${TARGET}.prg.dbg" -o ${TARGET}.prg

all: clean debug
	echo Done

move:
	mkdir -p obj
	rm obj/*.o || true
	mv *.o obj
	mkdir -p asm
	rm asm/*.asm || true
	mv *.asm asm

format1:
	cbmformat 8 1541SCAN,1

format2:
	cbmformat 8 1541SCAN,2

scratchall:
	cbmctrl command 8 "S0:*"
	cbmctrl status 8

copy:
	cbmcopy -w 8 build/1541scan.prg

clean:
	rm -Rf build