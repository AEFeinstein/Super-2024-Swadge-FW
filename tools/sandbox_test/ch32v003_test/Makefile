all : ../sandbox_upload run ../sandbox_interactive

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

SYSELF:=../../../build/swadge2024.elf

build : 
	mkdir -p build

sandbox.o : ../buildhelp sandbox.c $(SYSELF) build matrix_drops.h deep_sleep.h
	../buildhelp $(SYSELF) ../../..
	xtensa-esp32s2-elf-objdump -s build/sandbox.o > build/debug_sandbox_s.txt
	xtensa-esp32s2-elf-objdump -t build/sandbox.o > build/debug_sandbox_t.txt
	xtensa-esp32s2-elf-objdump -S build/sandbox.o > build/debug_sandbox_S.txt

run : ../sandbox_upload sandbox.o
	../sandbox_upload

../buildhelp : ../buildhelp.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

../sandbox_upload : ../sandbox_upload.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

../sandbox_interactive : ../sandbox_interactive.c
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

monitor : sandbox_interactive
	../sandbox_interactive

interactive : ../sandbox_interactive build
	../sandbox_interactive sandbox.c sandbox.S $(SYSELF)

CH32FLD:=../../ch32v003comp

matrix_drops.h : ../../../assets/ch32v003/matrix_drops.cfun
	riscv32-esp-elf-gcc -o build/$*.elf $(CH32FLD)/ch32fun.c $(CH32FLD)/swadge_matrix.c $(CH32FLD)/fakelibgcc.S -x c $< -g -Os -flto -ffunction-sections \
		-fdata-sections -fmessage-length=0 -msmall-data-limit=8 -march=rv32ec -mabi=ilp32e -DCH32V003 \
		-static-libgcc -I/usr/include/newlib -I. -T$(CH32FLD)/ch32v003.ld -I$(CH32FLD) -nostdlib -Wl,--print-memory-usage \
		-Wl,-Map=fwtest.map -Wl,--gc-sections
	riscv32-esp-elf-objcopy -R .storage  -O binary build/$*.elf build/$*.bin
	echo "unsigned char $*[] = { " > $@
	cat build/$*.bin | xxd -i >> $@
	echo "};" >> $@

deep_sleep.h : ../../../assets/ch32v003/deep_sleep.cfun
	riscv32-esp-elf-gcc -o build/$*.elf $(CH32FLD)/ch32fun.c $(CH32FLD)/swadge_matrix.c $(CH32FLD)/fakelibgcc.S -x c $< -g -Os -flto -ffunction-sections \
		-fdata-sections -fmessage-length=0 -msmall-data-limit=8 -march=rv32ec -mabi=ilp32e -DCH32V003 \
		-static-libgcc -I/usr/include/newlib -I. -T$(CH32FLD)/ch32v003.ld -I$(CH32FLD) -nostdlib -Wl,--print-memory-usage \
		-Wl,-Map=fwtest.map -Wl,--gc-sections
	riscv32-esp-elf-objcopy -R .storage  -O binary build/$*.elf build/$*.bin
	echo "unsigned char $*[] = { " > $@
	cat build/$*.bin | xxd -i >> $@
	echo "};" >> $@

clean :
	rm -rf *.o *~ buildhelp build/debug_sandbox_s.txt build/debug_sandbox_t.txt build/debug_sandbox_S.txt build/sandbox_inst.bin build/sandbox_data.bin build/buildhelp build/sandbox.o sandbox_upload build/sandbox.lds build/provided.lds build/sandbox_symbols.txt build/system_symbols.txt sandbox_interactive buildhelp.exe sandbox_upload.exe deep_sleep.h build/deep_sleep.bin build/deep_sleep.elf matrix_drops.h build/matrix_drops.bin build/matrix_drops.elf

