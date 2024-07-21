#include <stdio.h>
#include <stdint.h>

#include "../hidapi.h"
#include "../hidapi.c"

#define VID 0x1209
#define PID 0x4269

hid_device * hd;

#ifdef WIN32
const int chunksize = 244;
const int force_packet_length = 255;
const int reg_packet_length = 65;
BOOL CtrlCHandlerRoutine( DWORD dwCtrlType )
{
	if( dwCtrlType == 0)
	{
		exit( 0 );
	}
	printf( "dwCtrlType = %d\n", dwCtrlType );
	return FALSE;
}
#else
const int chunksize = 244;
const int force_packet_length = 255;
const int reg_packet_length = 64;
#endif

const int alignlen = 4;
int tries = 0;

int main()
{
    int do_for_tries = 100;

#ifdef WIN32
	SetConsoleCtrlHandler( CtrlCHandlerRoutine, 1 );
	HANDLE hConsole = GetStdHandle( STD_OUTPUT_HANDLE );
	system(""); // enable VT100 Escape Sequence for WINDOWS 10 Ver. 1607 
#endif

    int r;
redo:
    hid_init();

    hd = hid_open( VID, PID, 0);
    if( !hd )
    {
#ifdef WIN32
        Sleep( 2000 );
#else
        usleep(200000);
#endif
        if( do_for_tries < 80 )
        {
            fprintf( stderr, "Could not open USB (%04x:%04x)\n", VID, PID );
        }
        if( do_for_tries-- ) goto redo;
        return -94;
    }

    // Disable mode.
    uint8_t rdata[reg_packet_length];
    rdata[0] = 170;
    rdata[1] = 3; // AUSB_CMD_REBOOT
    rdata[2] = 1 & 0xff; // Yes, boot into bootloader.
    rdata[3] = 0 >> 8;
    rdata[4] = 0 >> 16;
    rdata[5] = 0 >> 24;
    do
    {
        r = hid_send_feature_report( hd, rdata, reg_packet_length );
        if( tries++ > 10 ) { fprintf( stderr, "Error sending feature report on command %d (%d)\n", rdata[1], r ); return -85; }
    } while ( r < 6 );
    tries = 0;

    hid_close( hd );
#ifdef WIN32
    Sleep( 2000 );
#else
    usleep(800000);
#endif
    return 0;
}

