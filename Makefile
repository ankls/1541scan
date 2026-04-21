SOURCES = $(abspath src/errors.c) \
          $(abspath src/disk.c) \
          $(abspath src/channel.c) \
		  $(abspath src/kernal_io.c) \
		  $(abspath src/opencbm_io.c) \
          $(abspath src/dos_io.c) \
          $(abspath src/display_aux.c) \
		  $(abspath src/keyboard.c) \
          $(abspath src/1541scan.c

TARGET = 1541scan

debug:
	mkdir -p build
	cd build && cl65 -Oir -g -I/usr/lib64/cc65/include -I../src -L/usr/lib64/cc65/include -I../src -L/usr/lib64/cc65  -vm -T -l 1541scan.asm -v $(SOURCES) -Wl "--mapfile,${TARGET}.map" -Wl "--dbgfile,${TARGET}.prg.dbg" -o ${TARGET}.prg

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