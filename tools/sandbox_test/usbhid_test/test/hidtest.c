#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <time.h>

#include "../../../hidapi.h"
#include "../../../hidapi.c"
#include "../../../../emulator/src/sound/os_generic.h"

#define VID 0x1209
#define PID 0x4269

#ifdef WIN32
const int reg_packet_length = 255;
const int reg_back_rest = 256;
#else
#include <unistd.h>
const int reg_packet_length = 254;
const int reg_back_rest = 254;
#endif

hid_device * hd;

int main( int argc, char ** argv )
{
	int r;

	hid_init();
	hd = hid_open( VID, PID, 0 );
	if( !hd ) { fprintf( stderr, "Could not open USB\n" ); return -94; }

	// Disable tick.
	uint8_t rdata[256] = { 0 };
	rdata[0] = 173;
	r = hid_get_feature_report( hd, rdata, reg_packet_length );
	printf( "Got data: %d bytes\n", r );
	int i;
	for( i = 0; i < r; i++ )
	{
		printf( "%02x ", rdata[i] );
	}
	printf( "\n" );

	double dStart = OGGetAbsoluteTime();
	for( i = 0; i < 1024; i++ )
	{
		r = hid_get_feature_report( hd, rdata, reg_packet_length );
		rdata[0] = 173;
		if( r != reg_back_rest )
		{
			fprintf( stderr, "Error reading message (%d)\n", r );
		}
	}
	double dEnd = OGGetAbsoluteTime();
	printf( "Reads: %5.0f/sec / %3.2f kB/s\n", 1024.0/(dEnd - dStart), (reg_packet_length)/(dEnd - dStart));

	dStart = OGGetAbsoluteTime();
	for( i = 0; i < 1024; i++ )
	{
		rdata[0] = 173;
		rdata[1] = 0x02;
		r = hid_send_feature_report( hd, rdata, reg_packet_length );
		if( r != reg_packet_length )
		{
			fprintf( stderr, "Error writing message (%d)\n", r );
		}
	}
	dEnd = OGGetAbsoluteTime();
	printf( "Writes: %5.0f/sec / %3.2f kB/s\n", 1024.0/(dEnd - dStart), (reg_packet_length)/(dEnd - dStart) );

	rdata[0] = 173;
	rdata[1] = 0x00;
	rdata[2] = 0xa5;
	rdata[3] = 0x5a;
	rdata[4] = 0xa5;
	rdata[5] = 0x5a;
	r = hid_send_feature_report( hd, rdata, reg_packet_length );
	printf( "Sent data %d / %02x %02x %02x\n", r, rdata[1], rdata[2], rdata[3] );

	int x, y;
	int f;
	for( f = 0; f < 10; f++ )
	{
		for( y = 0; y < 240; y+=10 )
		{
			for( x = 0; x < 280; x += 56 )
			{
				rdata[0] = 173;
				rdata[1] = 0x01;
				rdata[2] = x;
				rdata[3] = y;
				rdata[4] = 56;
				for( i = 0; i < 56; i++ )
					rdata[i+5] = rand();
				r = hid_send_feature_report( hd, rdata, reg_packet_length );
			}
		}
	}

	hid_close( hd );
}
