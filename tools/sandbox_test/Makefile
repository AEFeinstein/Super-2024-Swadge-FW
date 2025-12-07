all : sandbox_upload run sandbox_interactive

UNAME := $(shell uname)

ifeq ($(UNAME), Linux)
CFLAGS:=-g -O0
LDFLAGS:=-ludev
CC:=gcc
else
CFLAGS:=-Os -s
CC:=gcc
LDFLAGS:=C:/windows/system32/setupapi.dll
endif

SYSELF:=../../build/swadge2024.elf

build : 
	mkdir -p build

sandbox.o : buildhelp sandbox.c $(SYSELF) build
	./buildhelp $(SYSELF) ../..
	xtensa-esp32s2-elf-objdump -s build/sandbox.o > build/debug_sandbox_s.txt
	xtensa-esp32s2-elf-objdump -t build/sandbox.o > build/debug_sandbox_t.txt
	xtensa-esp32s2-elf-objdump -S build/sandbox.o > build/debug_sandbox_S.txt

run : sandbox_upload sandbox.o
	./sandbox_upload

buildhelp : buildhelp.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

sandbox_upload : sandbox_upload.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

sandbox_interactive : sandbox_interactive.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

monitor : sandbox_interactive
	./sandbox_interactive

interactive : sandbox_interactive build
	./sandbox_interactive sandbox.c sandbox.S $(SYSELF)

udevinstall :
	echo -ne "KERNEL==\"hidraw*\", SUBSYSTEM==\"hidraw\", ATTRS{idVendor}==\"1209\", ATTRS{idProduct}==\"4269\", GROUP=\"plugdev\", MODE=\"0660\"\nKERNEL==\"hidraw*\", SUBSYSTEM==\"hidraw\", ATTRS{idVendor}==\"1209\", ATTRS{idProduct}==\"4269\", GROUP=\"dialout\", MODE=\"0660\"" | sudo tee /etc/udev/rules.d/99-swadge.rules
	sudo udevadm control --reload-rules && sudo udevadm trigger


clean :
	rm -rf *.o *~ buildhelp build/debug_sandbox_s.txt build/debug_sandbox_t.txt build/debug_sandbox_S.txt build/sandbox_inst.bin build/sandbox_data.bin build/buildhelp build/sandbox.o sandbox_upload build/sandbox.lds build/provided.lds build/sandbox_symbols.txt build/system_symbols.txt sandbox_interactive buildhelp.exe sandbox_upload.exe

