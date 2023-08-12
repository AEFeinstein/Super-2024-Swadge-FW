#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#include "../hidapi.h"
#include "../hidapi.c"
#include <sys/stat.h>

#include <time.h>

#define VID 0x303a
#define PID 0x4004

#ifdef WIN32
const int chunksize = 244;
const int force_packet_length = 255;
const int reg_packet_length = 65;
#else
const int chunksize = 244;
const int force_packet_length = 255;
const int reg_packet_length = 64;
#endif


hid_device * hd;

int main( int argc, char ** argv )
{
	int first = 1;

	hid_init();
	hd = hid_open( VID, PID, 0 );
	if( !hd ) { fprintf( stderr, "Could not open USB [interactive]\n" ); return -94; }

#ifdef WIN32
	HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
	system(""); // enable VT100 Escape Sequence for WINDOWS 10 Ver. 1607 
#endif

	do
	{
		int r;

		// Disable tick.
		uint8_t rdata[513] = { 0 };
		rdata[0] = 172;
		r = hid_get_feature_report( hd, rdata, 513 );
#ifdef WIN32
		int toprint = r - 4;
#else
		int toprint = r - 2;
#endif

		if( r < 0 )
		{
			do
			{
				hd = hid_open( VID, PID, 0 );
				if( !hd )
					fprintf( stderr, "Could not open USB\n" );
				else
					fprintf( stderr, "Error: hid_get_feature_report failed with error %d\n", r );

				usleep( 100000 );
			} while( !hd );

			continue;
		}
		else if( toprint > 0 )
		{
			write( 1, rdata + 2, toprint );
		}
	} while( 1 );

	hid_close( hd );
}

