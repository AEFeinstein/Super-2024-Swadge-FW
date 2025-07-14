#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "os_generic.h"

int initCh32v003(int swdio_pin);
int ch32v003WriteMemory(const uint8_t* binary, uint32_t length, uint32_t address);
int ch32v003ReadMemory(uint8_t* binary, uint32_t length, uint32_t address);
int ch32v003GetReg(int regno, uint32_t* value);
int ch32v003SetReg(int regno, uint32_t regValue);
void ch32v003CheckTerminal();
void ch32v003Teardown();
int ch32v003Resume();
int ch32v003WriteFlash(const uint8_t* buf, int sz);

static inline void * ch32v003threadFn( void*v );
static inline uint32_t MINIRV32_LOAD4s( uint32_t ofs, uint32_t * rval, uint32_t * trap );
static inline uint16_t MINIRV32_LOAD2s( uint32_t ofs, uint32_t * rval, uint32_t * trap );
static inline uint8_t MINIRV32_LOAD1s( uint32_t ofs, uint32_t * rval, uint32_t * trap );
static inline int16_t MINIRV32_LOAD2_SIGNEDs( uint32_t ofs, uint32_t * rval, uint32_t * trap );
static inline int8_t MINIRV32_LOAD1_SIGNEDs( uint32_t ofs, uint32_t * rval, uint32_t * trap );


#define RAM_SIZE 2048
#define FLASH_SIZE 16384

uint8_t ch32v003flash[FLASH_SIZE];
uint8_t ch32v003ram[RAM_SIZE];

volatile int ch32v003runMode;
volatile int ch32v003quitMode;
struct MiniRV32IMAState ch32v003state;


#define MINI_RV32_RAM_SIZE (0x20000000+RAM_SIZE)
#define MINIRV32_RAM_IMAGE_OFFSET 0x00000000
#define MINIRV32WARN(x...) fprintf( stderr, x )
#define MINIRV32_MMIO_RANGE(n)  (0x40000000 <= (n) && (n) < 0x50000000)

#define MINIRV32_HANDLE_MEM_STORE_CONTROL( addy, val ) printf( "MMIO TODO STORE %08x\n", addy );

#define MINIRV32_HANDLE_MEM_LOAD_CONTROL( addy, val ) printf( "MMIO TODO LOAD %08x\n", addy );


#define MINIRV32_CUSTOM_MEMORY_BUS

#define MINIRV32_STORE4( ofs, val ) printf("LOAD4:%08x\n", ofs ); if( ofs < FLASH_SIZE - 3 ) { *(uint32_t*)(ch32v003flash + ofs) = val; }               else if( ofs >= 0x20000000 && ofs < 0x20000000 + RAM_SIZE - 3 ) { *(uint32_t*)(ch32v003ram + ofs) = val; } else { trap = (7+1); rval = ofs; }
#define MINIRV32_STORE2( ofs, val )  if( ofs < FLASH_SIZE - 1 ) { *(uint16_t*)(ch32v003flash + ofs) = val; }               else if( ofs >= 0x20000000 && ofs < 0x20000000 + RAM_SIZE - 1 ) { *(uint16_t*)(ch32v003ram + ofs) = val; } else { trap = (7+1); rval = ofs; }
#define MINIRV32_STORE1( ofs, val )  if( ofs < FLASH_SIZE - 0 ) { *(uint8_t* )(ch32v003flash + ofs) = val; }               else if( ofs >= 0x20000000 && ofs < 0x20000000 + RAM_SIZE - 0 ) { *(uint8_t* )(ch32v003ram + ofs) = val; } else { trap = (7+1); rval = ofs; }
static inline uint32_t MINIRV32_LOAD4s( uint32_t ofs, uint32_t * rval, uint32_t * trap )       {uint32_t tmp; if( ofs < FLASH_SIZE - 3 ) { tmp = *(uint32_t*)(ch32v003flash + ofs); } else if( ofs >= 0x20000000 && ofs < 0x20000000 + RAM_SIZE - 3 ) { tmp = *(uint32_t*)(ch32v003ram + ofs); } else { *trap = (7+1); *rval = ofs; } return tmp;}
static inline uint16_t MINIRV32_LOAD2s( uint32_t ofs, uint32_t * rval, uint32_t * trap )       {uint16_t tmp; if( ofs < FLASH_SIZE - 1 ) { tmp = *(uint16_t*)(ch32v003flash + ofs); } else if( ofs >= 0x20000000 && ofs < 0x20000000 + RAM_SIZE - 1 ) { tmp = *(uint16_t*)(ch32v003ram + ofs); } else { *trap = (7+1); *rval = ofs; } return tmp;}
static inline uint8_t MINIRV32_LOAD1s( uint32_t ofs, uint32_t * rval, uint32_t * trap )        {uint8_t tmp;  if( ofs < FLASH_SIZE - 0 ) { tmp = *(uint8_t* )(ch32v003flash + ofs); } else if( ofs >= 0x20000000 && ofs < 0x20000000 + RAM_SIZE - 0 ) { tmp = *(uint8_t* )(ch32v003ram + ofs); } else { *trap = (7+1); *rval = ofs; } return tmp;}
static inline int16_t MINIRV32_LOAD2_SIGNEDs( uint32_t ofs, uint32_t * rval, uint32_t * trap ) {int16_t tmp;  if( ofs < FLASH_SIZE - 1 ) { tmp = *(int16_t* )(ch32v003flash + ofs); } else if( ofs >= 0x20000000 && ofs < 0x20000000 + RAM_SIZE - 1 ) { tmp = *(int16_t* )(ch32v003ram + ofs); } else { *trap = (7+1); *rval = ofs; } return tmp;}
static inline int8_t MINIRV32_LOAD1_SIGNEDs( uint32_t ofs, uint32_t * rval, uint32_t * trap )  {int8_t tmp;   if( ofs < FLASH_SIZE - 0 ) { tmp = *(int8_t*  )(ch32v003flash + ofs); } else if( ofs >= 0x20000000 && ofs < 0x20000000 + RAM_SIZE - 0 ) { tmp = *(int8_t*  )(ch32v003ram + ofs); } else { *trap = (7+1); *rval = ofs; } return tmp;}

#define MINIRV32_LOAD4(ofs) MINIRV32_LOAD4s( ofs, &rval, &trap )
#define MINIRV32_LOAD2(ofs) MINIRV32_LOAD2s( ofs, &rval, &trap )
#define MINIRV32_LOAD1(ofs) MINIRV32_LOAD1s( ofs, &rval, &trap )
#define MINIRV32_LOAD2_SIGNED(ofs) MINIRV32_LOAD2_SIGNEDs( ofs, &rval, &trap )
#define MINIRV32_LOAD1_SIGNED(ofs) MINIRV32_LOAD1_SIGNEDs( ofs, &rval, &trap )

#define MINIRV32_IMPLEMENTATION

#include "mini-rv32ima.h"


og_thread_t ch32v003thread;

static void * ch32v003threadFn( void*v )
{
	memset( &ch32v003state, 0, sizeof(ch32v003state) );
	ch32v003runMode = 0;

	double dLast = OGGetAbsoluteTime();
	while( ch32v003quitMode == 0 )
	{
		double dNow = OGGetAbsoluteTime();
		uint32_t tus = (dNow - dLast)*1000000;
		if( ch32v003runMode )
		{
			printf( "%d %d\n", tus, 24*tus );
			MiniRV32IMAStep( &ch32v003state, 0, 0, tus, 24*tus );
		}
		OGUSleep(1);

		printf( "%08x %d\n", ch32v003state.pc, ch32v003runMode );

		dLast = dNow;
	}
	return 0;
}



int initCh32v003(int swdio_pin)
{
	ch32v003thread = OGCreateThread( ch32v003threadFn, 0 );
    return 0;
}

int ch32v003WriteMemory(const uint8_t* binary, uint32_t length, uint32_t address)
{
	uint32_t rval = 0, trap = 0;
	ch32v003runMode = 0;
	int i;
	for( i = 0; i < length; i++ )
		MINIRV32_STORE1( address+i, binary[i] );

    return (trap||rval) ? -1 : 0;
}

int ch32v003ReadMemory(uint8_t* binary, uint32_t length, uint32_t address)
{
	uint32_t rval = 0, trap = 0;
	ch32v003runMode = 0;
	int i;
	for( i = 0; i < length; i++ )
		binary[i] = MINIRV32_LOAD4( address+i );

    return (trap||rval) ? -1 : 0;
}

int ch32v003GetReg(int regno, uint32_t* value)
{
	// TODO
    return 0;
}

int ch32v003SetReg(int regno, uint32_t regValue)
{
	// TODO
    return 0;
}

void ch32v003CheckTerminal()
{
	// TODO
}

void ch32v003Teardown()
{
	ch32v003quitMode = true;
	OGJoinThread( ch32v003thread );
}

int ch32v003Resume()
{
	ch32v003runMode = 1;
    return 0;
}

int ch32v003WriteFlash(const uint8_t* buf, int sz)
{
    return ch32v003WriteMemory( buf, sz, 0 );
}


