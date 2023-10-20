#include <stdio.h>
#include <string.h>
#include "esp_system.h"
#include "swadge2024.h"
#include "hal/gpio_types.h"
#include "esp_log.h"
#include "soc/efuse_reg.h"
#include "soc/soc.h"
#include "soc/system_reg.h"
#include "hdw-tft.h"
#include "mainMenu.h"
#include "soc/rtc_cntl_reg.h"
#include "soc/gpio_reg.h"
#include "soc/io_mux_reg.h"
#include "rom/gpio.h"
#include "soc/i2c_reg.h"
#include "soc/gpio_struct.h"
#include "coreutil.h"
#include "hdw-btn.h"

#include "donut.h"
#include "hdw-imu.h"

int16_t verts_out[ sizeof(donut_verts)/3/2*3 ];

int frameno;
int bQuit;

#if 1

float rsqrtf(float x)
{
    typedef union
    {
        int32_t i;
        float f;
    } fiunion;
    const float xhalf = 0.5f * x;
    fiunion i         = {.f = x};
    i.i               = 0x5f375a86 - (i.i >> 1);
    x                 = i.f;
    x                 = x * (1.5f - xhalf * x * x);
    x                 = x * (1.5f - xhalf * x * x);
    return x;
}

void mathCrossProduct(float* p, const float* a, const float* b)
{
    float tx = a[1] * b[2] - a[2] * b[1];
    float ty = a[2] * b[0] - a[0] * b[2];
    p[2]     = a[0] * b[1] - a[1] * b[0];
    p[1]     = ty;
    p[0]     = tx;
}
#endif


#define FIXEDPOINT   16
#define FIXEDPOINTD2 15

void _drawTriangleOutlined(int16_t v0x, int16_t v0y, int16_t v1x, int16_t v1y, int16_t v2x, int16_t v2y,
                          paletteColor_t fillColor, paletteColor_t outlineColor)
{
    SETUP_FOR_TURBO();

    int16_t i16tmp;

    // Sort triangle such that v0 is the top-most vertex.
    // v0->v1 is LEFT edge.
    // v0->v2 is RIGHT edge.

    if (v0y > v1y)
    {
        i16tmp = v0x;
        v0x    = v1x;
        v1x    = i16tmp;
        i16tmp = v0y;
        v0y    = v1y;
        v1y    = i16tmp;
    }
    if (v0y > v2y)
    {
        i16tmp = v0x;
        v0x    = v2x;
        v2x    = i16tmp;
        i16tmp = v0y;
        v0y    = v2y;
        v2y    = i16tmp;
    }

    // v0 is now top-most vertex.  Now orient 2 and 3.
    // Tricky: Use slopes!  Otherwise, we could get it wrong.
    {
        int slope02;
        if (v2y - v0y)
        {
            slope02 = ((v2x - v0x) << FIXEDPOINT) / (v2y - v0y);
        }
        else
        {
            slope02 = ((v2x - v0x) > 0) ? 0x7fffff : -0x800000;
        }

        int slope01;
        if (v1y - v0y)
        {
            slope01 = ((v1x - v0x) << FIXEDPOINT) / (v1y - v0y);
        }
        else
        {
            slope01 = ((v1x - v0x) > 0) ? 0x7fffff : -0x800000;
        }

        if (slope02 < slope01)
        {
            i16tmp = v1x;
            v1x    = v2x;
            v2x    = i16tmp;
            i16tmp = v1y;
            v1y    = v2y;
            v2y    = i16tmp;
        }
    }

    // We now have a fully oriented triangle.
    int16_t x0A = v0x;
    int16_t y0A = v0y;
    int16_t x0B = v0x;
    // int16_t y0B = v0y;

    // A is to the LEFT of B.
    int dxA            = (v1x - v0x);
    int dyA            = (v1y - v0y);
    int dxB            = (v2x - v0x);
    int dyB            = (v2y - v0y);
    int sdxA           = (dxA > 0) ? 1 : -1;
    int sdyA           = (dyA > 0) ? 1 : -1;
    int sdxB           = (dxB > 0) ? 1 : -1;
    int sdyB           = (dyB > 0) ? 1 : -1;
    int xerrdivA       = (dyA * sdyA); // dx, but always positive.
    int xerrdivB       = (dyB * sdyB); // dx, but always positive.
    int xerrnumeratorA = 0;
    int xerrnumeratorB = 0;

    if (xerrdivA)
    {
        xerrnumeratorA = (((dxA * sdxA) << FIXEDPOINT) + xerrdivA / 2) / xerrdivA;
    }
    else
    {
        xerrnumeratorA = 0x7fffff;
    }

    if (xerrdivB)
    {
        xerrnumeratorB = (((dxB * sdxB) << FIXEDPOINT) + xerrdivB / 2) / xerrdivB;
    }
    else
    {
        xerrnumeratorB = 0x7fffff;
    }

    // X-clipping is handled on a per-scanline basis.
    // Y-clipping must be handled upfront.

    /*
        //Optimization BUT! Can't do this here, as we would need to be smarter about it.
        //If we do this, and the second triangle is above y=0, we'll get the wrong answer.
        if( y0A < 0 )
        {
            delta = 0 - y0A;
            y0A = 0;
            y0B = 0;
            x0A += (((xerrnumeratorA*delta)) * sdxA) >> FIXEDPOINT; //Could try rounding.
            x0B += (((xerrnumeratorB*delta)) * sdxB) >> FIXEDPOINT;
        }
    */

    {
        // Section 1 only.
        int yend = (v1y < v2y) ? v1y : v2y;
        int errA = 1 << FIXEDPOINTD2;
        int errB = 1 << FIXEDPOINTD2;
        int y;

        // Going between x0A and x0B
        for (y = y0A; y < yend; y++)
        {
            int x        = x0A;
            int endx     = x0B;
            int suppress = 1;

            if (y >= 0 && y < (int)TFT_HEIGHT)
            {
                suppress = 0;
                if (x < 0)
                {
                    x = 0;
                }
                if (endx > (int)(TFT_WIDTH))
                {
                    endx = (int)(TFT_WIDTH);
                }

                // Draw left line
                if (x0A >= 0 && x0A < (int)TFT_WIDTH)
                {
                    TURBO_SET_PIXEL(x0A, y, outlineColor);
                    x++;
                }

                // Draw body
                if (cTransparent != fillColor)
                {
                    for (; x < endx; x++)
                    {
                        TURBO_SET_PIXEL(x, y, fillColor);
                    }
                }

                // Draw right line
                if (x0B < (int)TFT_WIDTH && x0B >= 0)
                {
                    TURBO_SET_PIXEL(x0B, y, outlineColor);
                }
            }

            // Now, advance the start/end X's.
            errA += xerrnumeratorA;
            errB += xerrnumeratorB;
            while (errA >= (1 << FIXEDPOINT) && x0A != v1x)
            {
                x0A += sdxA;
                // if( x0A < 0 || x0A > (TFT_WIDTH-1) ) break;
                if (x0A >= 0 && x0A < (int)TFT_WIDTH && !suppress)
                {
                    TURBO_SET_PIXEL(x0A, y, outlineColor);
                }
                errA -= 1 << FIXEDPOINT;
            }
            while (errB >= (1 << FIXEDPOINT) && x0B != v2x)
            {
                x0B += sdxB;
                // if( x0B < 0 || x0B > (TFT_WIDTH-1) ) break;
                if (x0B >= 0 && x0B < (int)TFT_WIDTH && !suppress)
                {
                    TURBO_SET_PIXEL(x0B, y, outlineColor);
                }
                errB -= 1 << FIXEDPOINT;
            }
        }

        // We've come to the end of section 1.  Now, we need to figure

        // Now, yend is the highest possible hit on the triangle.

        // v1 is LEFT OF v2
        //  A is LEFT OF B
        if (v1y < v2y)
        {
            // V1 has terminated, move to V1->V2 but keep V0->V2[B] segment
            yend     = v2y;
            dxA      = (v2x - v1x);
            dyA      = (v2y - v1y);
            sdxA     = (dxA > 0) ? 1 : -1;
            xerrdivA = (dyA); // dx, but always positive.

            xerrnumeratorA = (((dxA * sdxA) << FIXEDPOINT) + xerrdivA / 2) / xerrdivA;

            x0A  = v1x;
            errA = 1 << FIXEDPOINTD2;
        }
        else
        {
            // V2 has terminated, move to V2->V1 but keep V0->V1[A] segment
            yend     = v1y;
            dxB      = (v1x - v2x);
            dyB      = (v1y - v2y);
            sdxB     = (dxB > 0) ? 1 : -1;
            sdyB     = (dyB > 0) ? 1 : -1;
            xerrdivB = (dyB * sdyB); // dx, but always positive.
            if (xerrdivB)
            {
                xerrnumeratorB = (((dxB * sdxB) << FIXEDPOINT) + xerrdivB / 2) / xerrdivB;
            }
            else
            {
                xerrnumeratorB = 0x7fffff;
            }
            x0B  = v2x;
            errB = 1 << FIXEDPOINTD2;
        }

        if (yend > (int)(TFT_HEIGHT - 1))
        {
            yend = (int)TFT_HEIGHT - 1;
        }

        if (xerrnumeratorA > 1000000 || xerrnumeratorB > 1000000)
        {
            if (x0A < x0B)
            {
                sdxA = 1;
                sdxB = -1;
            }
            if (x0A > x0B)
            {
                sdxA = -1;
                sdxB = 1;
            }
            if (x0A == x0B)
            {
                if (x0A >= 0 && x0A < (int)TFT_WIDTH && y >= 0 && y < (int)TFT_HEIGHT)
                {
                    TURBO_SET_PIXEL(x0A, y, outlineColor);
                }
                return;
            }
        }

        for (; y <= yend; y++)
        {
            int x        = x0A;
            int endx     = x0B;
            int suppress = 1;

            if (y >= 0 && y <= (int)(TFT_HEIGHT - 1))
            {
                suppress = 0;
                if (x < 0)
                {
                    x = 0;
                }
                if (endx >= (int)(TFT_WIDTH))
                {
                    endx = (TFT_WIDTH);
                }

                // Draw left line
                if (x0A >= 0 && x0A < (int)(TFT_WIDTH))
                {
                    TURBO_SET_PIXEL(x0A, y, outlineColor);
                    x++;
                }

                // Draw body
                if (cTransparent != fillColor)
                {
                    for (; x < endx; x++)
                    {
                        TURBO_SET_PIXEL(x, y, fillColor);
                    }
                }

                // Draw right line
                if (x0B < (int)(TFT_WIDTH) && x0B >= 0)
                {
                    TURBO_SET_PIXEL(x0B, y, outlineColor);
                }
            }

            // Now, advance the start/end X's.
            errA += xerrnumeratorA;
            errB += xerrnumeratorB;
            while (errA >= (1 << FIXEDPOINT))
            {
                x0A += sdxA;
                // if( x0A < 0 || x0A > (TFT_WIDTH-1) ) break;
                if (x0A >= 0 && x0A < (int)(TFT_WIDTH) && !suppress)
                {
                    TURBO_SET_PIXEL(x0A, y, outlineColor);
                }
                errA -= 1 << FIXEDPOINT;
                if (x0A == x0B)
                {
                    return;
                }
            }
            while (errB >= (1 << FIXEDPOINT))
            {
                x0B += sdxB;
                if (x0B >= 0 && x0B < (int)(TFT_WIDTH) && !suppress)
                {
                    TURBO_SET_PIXEL(x0B, y, outlineColor);
                }
                errB -= 1 << FIXEDPOINT;
                if (x0A == x0B)
                {
                    return;
                }
            }
        }
    }
}


static unsigned julery_isqrt(unsigned long val) {
    unsigned long temp, g=0, b = 0x8000, bshft = 15;
    do {
        if (val >= (temp = (((g << 1) + b)<<bshft--))) {
           g += b;
           val -= temp;
        }
    } while (b >>= 1);
    return g;
}



int global_i = 100;
menu_t * menu;
const char * menu_Bootload = "Bootloader";

// External functions defined in .S file for you assembly people.
void minimal_function();
uint32_t test_function( uint32_t x );
uint32_t asm_read_gpio();

wsg_t example_sprite;

static void mainMenuCb(const char* label, bool selected, uint32_t settingVal)
{
	if( label == mainMenuMode.modeName )
	{
		switchToSwadgeMode( &mainMenuMode );
	}
	else if( label == menu_Bootload )
	{
		// Uncomment this to reboot the chip into the bootloader.
		// This is to test to make sure we can call ROM functions.
		REG_WRITE(RTC_CNTL_OPTION1_REG, RTC_CNTL_FORCE_DOWNLOAD_BOOT);
		void software_reset( uint32_t x );
		software_reset( 0 );
	}
}

void intcross(int* p, const int* a, const int* b)
{
    float tx = a[1] * b[2] - a[2] * b[1];
    float ty = a[2] * b[0] - a[0] * b[2];
    p[2]     = a[0] * b[1] - a[1] * b[0];
    p[1]     = ty;
    p[0]     = tx;
}

void sandbox_main(void)
{
	frameno = 0;
	bQuit = 0;

	ESP_LOGI( "sandbox", "Running from IRAM. %d", global_i );

	REG_WRITE( GPIO_FUNC7_OUT_SEL_CFG_REG,4 ); // select ledc_ls_sig_out0

	menu = initMenu("USB Sandbox", mainMenuCb);
	addSingleItemToMenu(menu, mainMenuMode.modeName);
	addSingleItemToMenu(menu, menu_Bootload);

	loadWsg("kid0.wsg", &example_sprite, true);

	accelSetRegistersAndReset();

	setFrameRateUs(5000);

	ESP_LOGI( "sandbox", "Loaded" );
}

void sandbox_exit()
{
	bQuit = 1;
}

int zcompare(const int16_t *a, const int16_t* b)
{
	return a[0] - b[0];
}

void sandbox_tick()
{
	if( bQuit ) return;


	buttonEvt_t evt = {0};
	while (checkButtonQueueWrapper(&evt))
	{
		menu = menuButton(menu, evt);
	}


	char ctsbuffer[1024];
	char *cts = ctsbuffer;


	float plusy[3] = { 0, 1, 0 };

	// Produce a model matrix from a quaternion.
	float plusx_out[3] = { 0.9, 0, 0 };
	float plusy_out[3] = { 0, 0.9, 0 };
	float plusz_out[3] = { 0, 0, 0.9 };
	mathRotateVectorByQuaternion( plusy, LSM6DSL.fqQuat, plusy );
	mathRotateVectorByQuaternion( plusy_out, LSM6DSL.fqQuat, plusy_out );
	mathRotateVectorByQuaternion( plusx_out, LSM6DSL.fqQuat, plusx_out );
	mathRotateVectorByQuaternion( plusz_out, LSM6DSL.fqQuat, plusz_out );

	uint32_t cycStart = getCycleCount();

	int i, vertices = 0;
	for( i = 0; i < sizeof(donut_verts); i+= 3 )
	{
		// Performingthe transform this way is about 700us.
		float bx = donut_verts[i+0];
		float by = donut_verts[i+1];
		float bz = donut_verts[i+2];
		float bunnyvert[3] = {
			bx * plusx_out[0] + by * plusx_out[1] + bz * plusx_out[2],
			bx * plusy_out[0] + by * plusy_out[1] + bz * plusy_out[2],
			bx * plusz_out[0] + by * plusz_out[1] + bz * plusz_out[2] };
		verts_out[vertices*3+0] = bunnyvert[0] + 280/2; 
		verts_out[vertices*3+1] =-bunnyvert[1] + 240/2;  // Convert from right-handed to left-handed coordinate frame.
		verts_out[vertices*3+2] = bunnyvert[2];
		vertices++;
	}

	int totalTrisThisFrame = 0;
	// ID, Z
	static int16_t trimap[sizeof(donut_tris)/4][3];


	for( i = 0; i < sizeof(donut_tris); i+= 4)
	{
		int tv1 = donut_tris[i]*3;
		int tv2 = donut_tris[i+1]*3;
		int tv3 = donut_tris[i+2]*3;
		int col = donut_tris[i+3];

		int diff1[3] = {
			verts_out[tv3+0] - verts_out[tv1+0],
			verts_out[tv3+1] - verts_out[tv1+1],
			verts_out[tv3+2] - verts_out[tv1+2] };
		int diff2[3] = {
			verts_out[tv2+0] - verts_out[tv1+0],
			verts_out[tv2+1] - verts_out[tv1+1],
			verts_out[tv2+2] - verts_out[tv1+2] };

		// If we didn't need the normal, could do cross faster. int crossproduct = diff1[1] * diff2[0] - diff1[0] * diff2[1];

		int icrp[3];
		intcross( icrp, diff1, diff2 );
		if( icrp[2] < 0 ) continue;
		int z = verts_out[tv1+2] + verts_out[tv2+2] + verts_out[tv3+2];

		int b = col % 6;
		int g = ( col / 6 ) % 6;
		int r = ( col / 36 ) % 6;

		//float fcrp[3] = { icrp[0], icrp[1], icrp[2] };
		int crpscalar = julery_isqrt( icrp[0] * icrp[0] + icrp[1] * icrp[1] + icrp[2] * icrp[2] );
		icrp[0] = ( 1024 * icrp[0] ) / crpscalar;
		icrp[1] = ( 1024 * icrp[1] ) / crpscalar;
		icrp[2] = ( 1024 * icrp[2] ) / crpscalar;

		int isum = icrp[0] - icrp[1] + icrp[2];

		r = ( r * ( ( isum ) + 1200 ) * 100 ) >> 18;
		g = ( g * ( ( isum ) + 1200 ) * 100 ) >> 18;
		b = ( b * ( ( isum ) + 1200 ) * 100 ) >> 18;

		if( r < 0 ) r = 0;
		if( g < 0 ) g = 0;
		if( b < 0 ) b = 0;
		if( r > 5 ) r = 5;
		if( g > 5 ) g = 5;
		if( b > 5 ) b = 5;

		trimap[totalTrisThisFrame][0] = z;
		trimap[totalTrisThisFrame][1] = i;
		trimap[totalTrisThisFrame][2] = r * 36 + g * 6 + b;
		totalTrisThisFrame++;
	}

	qsort(trimap, totalTrisThisFrame, sizeof( trimap[0] ), (void*)zcompare );

	int lines = 0;
	for( i = 0; i < totalTrisThisFrame; i++)
	{
		int j = trimap[i][1];
		int tv1 = donut_tris[j]*3;
		int tv2 = donut_tris[j+1]*3;
		int tv3 = donut_tris[j+2]*3;
		int tcol = trimap[i][2];

		_drawTriangleOutlined(
			verts_out[tv1+0], verts_out[tv1+1],
			verts_out[tv2+0], verts_out[tv2+1],
			verts_out[tv3+0], verts_out[tv3+1],
			tcol, tcol );
	}

	uint32_t renderTime = getCycleCount() - cycStart;


	ESP_LOGI( "I2C", "%d %d %d",
		(int)renderTime, (int)verts_out[2], totalTrisThisFrame );

/*
	cts += sprintf( cts, "%ld %ld %d %f %f %f / %f %f %f / %3d %3d %3d / %4d %4d %4d",
		LSM6DSL.computetime, renderTime, LSM6DSL.lastreadr,
		LSM6DSL.fCorrectLast[0], LSM6DSL.fCorrectLast[1], LSM6DSL.fCorrectLast[2],
		LSM6DSL.fvBias[0],		LSM6DSL.fvBias[1],		LSM6DSL.fvBias[2],
		LSM6DSL.gyrolast[0], LSM6DSL.gyrolast[1], LSM6DSL.gyrolast[2],
		LSM6DSL.accellast[0], LSM6DSL.accellast[1], LSM6DSL.accellast[2] );

	ESP_LOGI( "I2C", "%s", ctsbuffer );
*/
}

void sandboxBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum )
{
//	accelIntegrate();
	if( up == 2 || up == 8 || up == 13 ) 
		accelIntegrate();

	fillDisplayArea(x, y, x+w, y+h, 2 );
}


swadgeMode_t sandbox_mode = {
	.modeName				 = "sandbox",
	.wifiMode				 = NO_WIFI,
	.overrideUsb			  = false,
	.usesAccelerometer		= false,
	.usesThermometer		  = false,
	.fnEnterMode			  = sandbox_main,
	.fnExitMode			   = sandbox_exit,
	.fnMainLoop			   = sandbox_tick,
	.fnAudioCallback		  = NULL,
	.fnBackgroundDrawCallback = sandboxBackgroundDrawCallback,
	.fnEspNowRecvCb		   = NULL,
	.fnEspNowSendCb		   = NULL,
	.fnAdvancedUSB			= NULL
};


