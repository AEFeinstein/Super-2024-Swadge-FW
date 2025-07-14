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

#if defined( WIN32 ) || defined (WINDOWS) || defined( _WIN32)
static double OGGetFileTime( const char * file )
{
	FILETIME ft;
	HANDLE h = CreateFile(file, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if( h==INVALID_HANDLE_VALUE )
		return -1;
	GetFileTime( h, 0, 0, &ft );
	CloseHandle( h );
	return ft.dwHighDateTime + ft.dwLowDateTime;
}
#else
#include <sys/stat.h>
static double OGGetFileTime( const char * file )
{
	struct stat buff; 
	int r = stat( file, &buff );
	if( r < 0 )
		return -1;
	return buff.st_mtime;
}
#endif

bool process_cfun(processorInput_t* arg);

const assetProcessor_t cfunProcessor
    = {.name = "cfun", .type = FUNCTION, .function = process_cfun, .inFmt = FMT_FILENAME, .outFmt = FMT_FILENAME};

bool process_cfun(processorInput_t* arg)
{
	char cfunname[1024];
	char bincfunname[1024];
	snprintf( cfunname, sizeof(cfunname), "%s", arg->in.fileName );
	snprintf( bincfunname, sizeof(bincfunname), "%s.bin", arg->in.fileName );

	double timeInSrc = OGGetFileTime( cfunname );
	double timeInBin = OGGetFileTime( bincfunname );

    char ch32v003comp[3096];
	sprintf( ch32v003comp, "%s/../tools/ch32v003comp", outDirName );

    char compile_line[16384];

	if( timeInSrc >= timeInBin )
	{
		char * idf = getenv("IDF_PATH");

		if( !idf || *idf == 0 )
		{
		    fprintf( stderr, "Error: Need IDF_PATH, otherwise we won't be able to get the RISC-V compiler.  Please export your ESP-IDF.\n" );
		    return false;
		}

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
		    "riscv32-esp-elf-objcopy -O binary %s.elf %s.bin", arg->in.fileName, arg->in.fileName );

		printf( "$ %s\n", compile_line );
		if( system( compile_line ) )
		{
		    fprintf( stderr, "Cannot export binary.\n" );
		    return false;
		}
	}
	else
	{
		printf( "%s binary appears up to date.\n", arg->in.fileName );
	}

	snprintf( compile_line, sizeof(compile_line),
		"cp %s.bin %s.bin", arg->in.fileName, arg->out.fileName );

    printf( "$ %s\n", compile_line );
    if( system( compile_line ) )
    {
        fprintf( stderr, "Cannot copy binaries\n" );
        return false;
    }

    return true;
}

