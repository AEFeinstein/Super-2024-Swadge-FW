#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#include "assets_preprocessor.h"
#include "cfun_processor.h"

#include "fileUtils.h"

bool process_cfun(processorInput_t* arg);

const assetProcessor_t cfunProcessor
    = {.name = "cfun", .type = FUNCTION, .function = process_cfun, .inFmt = FMT_FILENAME, .outFmt = FMT_FILENAME};

bool process_cfun(processorInput_t* arg)
{
    char * idf = getenv("IDF_PATH");

    if( !idf || *idf == 0 )
    {
        fprintf( stderr, "Error: Need IDF_PATH, otherwise we won't be able to get the RISC-V compiler.  Please export your ESP-IDF.\n" );
        return false;
    }

    printf( "::%s\n", outDirName );

    char ch32v003comp[3096];
	sprintf( ch32v003comp, "%s/../tools/ch32v003comp", outDirName );

    char compile_line[16384];
    snprintf( compile_line, sizeof(compile_line),
        "riscv32-esp-elf-gcc -o %s.elf %s/ch32fun.c %s/fakelibgcc.S -x c %s -g -Os -flto -ffunction-sections \
         -fdata-sections -fmessage-length=0 -msmall-data-limit=8 -march=rv32ec -mabi=ilp32e -DCH32V003 \
         -static-libgcc -I/usr/include/newlib -I. -T%s/ch32v003.ld -I%s -nostdlib -Wl,--print-memory-usage \
         -Wl,-Map=%s.map -Wl,--gc-sections",
        arg->in.fileName,
        ch32v003comp,
        ch32v003comp,
        arg->in.fileName,
        ch32v003comp,
        ch32v003comp,
        arg->in.fileName );

    printf( "$ %s\n", compile_line );
    if( system( compile_line ) )
    {
        fprintf( stderr, "Compilation failed.\n" );
        return false;
    }

    snprintf( compile_line, sizeof(compile_line),
        "riscv32-esp-elf-objdump -S %s.elf > %s.lst", arg->in.fileName, arg->in.fileName );

    printf( "$ %s\n", compile_line );
    if( system( compile_line ) )
    {
        fprintf( stderr, "Cannot export listing.\n" );
        return false;
    }

    snprintf( compile_line, sizeof(compile_line),
        "riscv32-esp-elf-objcopy -O binary %s.elf %s.bin", arg->in.fileName, arg->out.fileName );

    printf( "$ %s\n", compile_line );
    if( system( compile_line ) )
    {
        fprintf( stderr, "Cannot export binary.\n" );
        return false;
    }

    return true;
}
