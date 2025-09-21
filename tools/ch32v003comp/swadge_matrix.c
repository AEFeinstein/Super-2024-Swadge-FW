// This code is only compiled for the ch32v003. It is for running
// on the swadge coprocessor.

//==============================================================================
// Includes
//==============================================================================

#include "swadge_matrix.h"

//==============================================================================
// Variables
//==============================================================================

uint8_t LEDSets[9*8];
const uint16_t Coordmap[8*16] = {
    0x0000, 0x0100, 0x0200, 0x0300, 0x0400, 0x0500, 0xffff, 0xffff,
    0x0002, 0x0102, 0x0202, 0x0302, 0x0402, 0x0502, 0xffff, 0xffff,
    0x0001, 0x0101, 0x0201, 0x0301, 0x0401, 0x0501, 0xffff, 0xffff,
    0x0008, 0x0108, 0x0208, 0x0308, 0x0408, 0x0508, 0xffff, 0xffff,
    0x0007, 0x0107, 0x0207, 0x0307, 0x0407, 0x0507, 0xffff, 0xffff,
    0x0006, 0x0106, 0x0206, 0x0306, 0x0406, 0x0506, 0xffff, 0xffff,
    0x0005, 0x0205, 0x0405, 0x0605, 0x0600, 0x0700, 0xffff, 0xffff,
    0x0004, 0x0204, 0x0404, 0x0604, 0x0602, 0x0702, 0xffff, 0xffff,
    0x0003, 0x0203, 0x0403, 0x0603, 0x0601, 0x0701, 0xffff, 0xffff,
    0x0105, 0x0305, 0x0505, 0x0705, 0x0608, 0x0708, 0xffff, 0xffff,
    0x0104, 0x0304, 0x0504, 0x0704, 0x0607, 0x0707, 0xffff, 0xffff,
    0x0103, 0x0303, 0x0503, 0x0703, 0x0606, 0x0706, 0xffff, 0xffff,
};

//==============================================================================
// Functions
//==============================================================================

/**
 * @brief Draw a pixel on the matrix at x,y with appropriate intensity.
 *
 * This handles writing the correct values into RAM for the DMA engine to display.
 *
 * @param x the X coordinate on the matrix to overwrite.
 * @param y the Y coordinate on the matrix to overwrite.
 * @param intensity the brightness of the LED to draw.
 */
void SetPixel( int x, int y, int intensity )
{
    int i;

    int coord = Coordmap[x*8+y];

    int ox = coord & 0xff;
    int oy = coord >> 8;

    int ofs = ox;
    uint8_t * ledo = &LEDSets[ofs];
    int imask = ~(1<<oy);
    int mask = ~imask;

    for( i = 0; i < 8; i++ )
    {
        if( intensity & (1<<i) )
            *ledo |= mask;
        else
            *ledo &= imask;
        ledo += 9;
    }

#if 0
    for( i = MAX_INTENSITY-1; i >= intensity; i-- )
    {
        *ledo &= imask;
        ledo += 9;
    }
    for( ; i >= 0; i-- )
    {
        *ledo |= mask;
        ledo += 9;
    }
#endif
}


/**
 * @brief Fast approximate integer square root.
 *
 * This is optimized for systems that have Zbb extension, but is still pretty fast on others.
 *
 * @param i the number to take the square root of
 * @return the square root of i
 */
int apsqrt( int i )
{
    if( i == 0 ) return 0;
    int x = 1<<( ( 32 - __builtin_clz(i) )/2);
    x = (x + i/x)/2;
//    x = (x + i/x)/2; //May be needed depending on how precise you want. (Below graph is without this line)
    return x;
}


/**
 * @brief Configure ch32v003 hardware
 *
 * This sets up the ch32v003, and configures the DMA engine for outputting to
 * the hardware DMA matrix, as well as giving the swadge emulator the sentinel
 * for the display matrix output.
 *
 */
void MatrixSetup()
{
    SystemInit();
    // Enable DMA
    RCC->AHBPCENR = RCC_AHBPeriph_SRAM | RCC_AHBPeriph_DMA1;

    // Enable timers and remapping.
    RCC->APB2PCENR = RCC_APB2Periph_TIM1 | RCC_APB2Periph_AFIO | RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOD;
    RCC->APB1PCENR = RCC_APB1Periph_TIM2;

    // First make sure we configure PD7 as an output (GPIO)
    FLASH->OBKEYR = FLASH_KEY1;
    FLASH->OBKEYR = FLASH_KEY2;
    FLASH->KEYR = FLASH_KEY1;
    FLASH->KEYR = FLASH_KEY2;
    FLASH->MODEKEYR = FLASH_KEY1;
    FLASH->MODEKEYR = FLASH_KEY2;

    uint16_t rdptmp = RDP_Key;

    int status = FLASH_WaitForLastOperation(EraseTimeout);
    if(status == FLASH_COMPLETE)
    {
        FLASH->OBKEYR = FLASH_KEY1;
        FLASH->OBKEYR = FLASH_KEY2;

        FLASH->CTLR |= CR_OPTER_Set;
        FLASH->CTLR |= CR_STRT_Set;
        status = FLASH_WaitForLastOperation(EraseTimeout);

        if(status == FLASH_COMPLETE)
        {
            FLASH->CTLR &= CR_OPTER_Reset;
            FLASH->CTLR |= CR_OPTPG_Set;
            OB->RDPR = (uint16_t)rdptmp;
            status = FLASH_WaitForLastOperation(ProgramTimeout);

            if(status != FLASH_TIMEOUT)
            {
                FLASH->CTLR &= CR_OPTPG_Reset;
            }
        }
        else
        {
            if(status != FLASH_TIMEOUT)
            {
                FLASH->CTLR &= CR_OPTPG_Reset;
            }
        }
    }

    FLASH->OBKEYR = FLASH_KEY1;
    FLASH->OBKEYR = FLASH_KEY2;
    status = FLASH_WaitForLastOperation(10000);

    const uint16_t OB_STOP = OB_STOP_NoRST;
    const uint16_t OB_IWDG = OB_IWDG_SW;
    const uint16_t OB_STDBY = OB_STDBY_NoRST;
    const uint16_t OB_RST = OB_RST_NoEN; // Disable resetting on reset pin
    const uint16_t OB_BOOT = OB_STARTMODE_BOOT;

    FLASH->OBKEYR = FLASH_KEY1;
    FLASH->OBKEYR = FLASH_KEY2;
    status = FLASH_WaitForLastOperation(10000);

    if(status == FLASH_COMPLETE)
    {
        FLASH->CTLR |= CR_OPTPG_Set;
        OB->USER = OB_BOOT | OB_IWDG | (uint16_t)(OB_STOP | (uint16_t)(OB_STDBY | (uint16_t)(OB_RST | (uint16_t)0xc0)));

        status = FLASH_WaitForLastOperation(10000);
        if(status != FLASH_TIMEOUT)
        {
            FLASH->CTLR &= CR_OPTPG_Reset;
        }
    }

    // Option byte writing done.


    GPIOC->CFGLR = 0x22222222; // GPIO_CFGLR_IN_FLOAT = 4, 
    GPIOD->CFGLR = 0x22222242; // GPIO_CFGLR_OUT_2Mhz_PP = 2
    GPIOA->CFGLR = 0x220; // GPIO_CFGLR_OUT_2Mhz_PP = 2

    // Configure default output values.
    GPIOA->BSHR = 6;
    GPIOD->BSHR = 0xfd;

    //static uint8_t As[] = { PD0, PA1, PA2, PD2, PD3, PD4, PD5, PD6, PD7 };
    static volatile uint32_t * RowControls[] = { &GPIOD->OUTDR, &GPIOA->OUTDR, &GPIOA->OUTDR, &GPIOD->OUTDR, &GPIOD->OUTDR, &GPIOD->OUTDR, &GPIOD->OUTDR, &GPIOD->OUTDR, &GPIOD->OUTDR };
    static uint8_t    RowValue[] = { ~0x01, 0xff, ~0x02, 0xff, ~0x04, 0xff, ~0x04, 0xff, ~0x08, 0xff, ~0x10, 0xff, ~0x20, 0xff, ~0x40, 0xff, 0x7f,  0xff };

    #define CLONESET(x)   x, x, x, x, x, x, x, x, x
    #define CLONESETM2(x) x, x, x, x, x, x, x
    static uint16_t    PrescaleChange[] = { 
        CLONESETM2(1), CLONESET(3), CLONESET(7), CLONESET(15), CLONESET(31), CLONESET(63), CLONESET(127), CLONESET(255), 1, 1
    };
    //3, 7, 15, 31, 63, 127, 255, 511 };

    // T2C3 is the DMA->DMA Control, DMAC1.
    //      DMAC1 reads from RowControls and sets the OUTDR for DMAC7.
    // T1C2/C4 is the DMA->ROW Control, DMAC7
    //    This trigegrs 2x as fast as the others, and sets a row.
    // T2C1 is the DMA->PC Control, DMAC5
    //    This outputs to a Column.  I.e. Port C.
    // T2U is the timer to change the timer speed in DMAC2

    DMA1_Channel1->CNTR = 9;
    DMA1_Channel1->MADDR = (uint32_t)&RowControls[0];
    DMA1_Channel1->PADDR = (uint32_t)&DMA1_Channel7->PADDR;
    DMA1_Channel1->CFGR = DMA_DIR_PeripheralDST | DMA_CFGR1_PL /* Highest Priority */ | DMA_CFGR1_MINC | DMA_CFGR1_CIRC | DMA_CFGR1_EN | DMA_CFGR1_MSIZE_1 | DMA_CFGR1_PSIZE_1;

    DMA1_Channel7->CNTR = 18;
    DMA1_Channel7->MADDR = (uint32_t)&RowValue[0];
    //DMA1_Channel3->PADDR = // Configured by DMAC5
    DMA1_Channel7->CFGR = DMA_DIR_PeripheralDST | DMA_CFGR1_PL_0 | DMA_CFGR1_MINC | DMA_CFGR1_CIRC | DMA_CFGR1_EN /* 8-bit in, 8-bit out */;

    DMA1_Channel5->CNTR = sizeof(LEDSets);
    DMA1_Channel5->MADDR = (uint32_t)LEDSets;
    DMA1_Channel5->PADDR = (uint32_t)&GPIOC->OUTDR;
    DMA1_Channel5->CFGR = DMA_DIR_PeripheralDST | DMA_CFGR1_PL_0 | DMA_CFGR1_MINC | DMA_CFGR1_CIRC | DMA_CFGR1_EN /* 8-bit in, 8-bit out */;

    DMA1_Channel2->CNTR = sizeof(PrescaleChange)/sizeof(PrescaleChange[0]);
    DMA1_Channel2->MADDR = (uint32_t)PrescaleChange;
    DMA1_Channel2->PADDR = (uint32_t)&TIM2->PSC;
    DMA1_Channel2->CFGR = DMA_DIR_PeripheralDST | DMA_CFGR1_PL /* Highest Priority*/ | DMA_CFGR1_MINC | DMA_CFGR1_CIRC | DMA_CFGR1_EN | DMA_MemoryDataSize_HalfWord | DMA_PeripheralDataSize_HalfWord /* 8-bit in, 8-bit out */;

    TIM2->PSC = 0x4;      // Prescaler  (Fastest we can reliably go without race conditions) (3 works _almost_ all the time, 4 is always reliable)
    TIM2->ATRLR = 20;       // Auto Reload - sets period  (This is how fast each pixel works per set)
    TIM2->SWEVGR = TIM_UG;     // Reload immediately (This does not seem to be working?)  see the M2 we need to do in CLONESETM2?  But not having it is worse.
    TIM2->DMAINTENR = TIM_COMDE | TIM_TDE | TIM_UDE | TIM_CC2DE | TIM_CC1DE | TIM_CC3DE | TIM_CC4DE;
    TIM2->CH1CVR = 2;
    TIM2->CH2CVR = 4;
    TIM2->CH3CVR = 1;
    TIM2->CH4CVR = 20; // This must be ATLAR.
    TIM2->CNT = 0;
    TIM2->CTLR1 |= TIM_CEN; // Enable

    // Trigger TIM1 with TIM2  We just use TIM1 like a frame counter.
    // ITR0(TS=000) => From timer 2 to timer 1.
    // This triggers one bit per 

    TIM1->PSC = 0;
    TIM1->ATRLR = 65535;
    TIM1->SWEVGR = TIM_UG;     // Reload immediately
    TIM1->DMAINTENR = 0;
    TIM1->CNT = 0;
    TIM1->SMCFGR = TIM_SMS | TIM_TS_2 | TIM_TS_1; // TS = 110: Filtered timer input 2 (TI2FP2).  SMS = Mode 7.
    TIM1->CTLR1 |= TIM_CEN; // Enable
}


int FLASH_GetBank1Status(void)
{
    int flashstatus = FLASH_COMPLETE;

    if((FLASH->STATR & FLASH_FLAG_BANK1_BSY) == FLASH_FLAG_BSY)
    {
        flashstatus = FLASH_BUSY;
    }
    else
    {
        if((FLASH->STATR & FLASH_FLAG_BANK1_WRPRTERR) != 0)
        {
            flashstatus = FLASH_ERROR_WRP;
        }
        else
        {
            flashstatus = FLASH_COMPLETE;
        }
    }
    return flashstatus;
}


int FLASH_WaitForLastOperation(uint32_t Timeout)
{
    int status = FLASH_COMPLETE;

    status = FLASH_GetBank1Status();
    while((status == FLASH_BUSY) && (Timeout != 0x00))
    {
        status = FLASH_GetBank1Status();
        Timeout--;
    }
    if(Timeout == 0x00)
    {
        status = FLASH_TIMEOUT;
    }
    return status;
}
