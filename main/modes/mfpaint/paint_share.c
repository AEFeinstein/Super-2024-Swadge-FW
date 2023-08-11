#include "paint_share.h"

#include <stddef.h>
#include <string.h>

#include "p2pConnection.h"
#include "display.h"
#include "bresenham.h"

#include "mode_paint.h"
#include "paint_common.h"
#include "paint_nvs.h"
#include "paint_util.h"

/**
 * Share mode!
 *
 * So, how will this work?
 * On the SENDING swadge:
 * - Select "Share" mode from the paint menu
 * - The image from the most recent slot is displayed (at a smaller scale than in draw mode)
 * - The user can page with Left and Right, or Select to switch between used slots
 * - The user can begin sharing by pressing A or Start
 * - Once sharing begins, the swadge opens a P2P connection
 * - When a receiving swadge is found, sharing begins immediately.
 * - The sender sends a metadata packet, which includes canvas dimensions, and palette
 * - We wait for confirmation that the metadata was acked (TODO: and handled properly.)
 * - Now, we begin sending packets. We wait until each one is ACKed before sending another (TODO: don't send another packet until the receiver asks for more)
 * - Each packet contains an absolute sequence number and as many bytes of pixel data as will fit (palette-indexed and packed into 2 pixels per byte)
 * - Once the last packet has been acked, we're done! Return to share mode
 */

#define SHARE_LEFT_MARGIN 10
#define SHARE_RIGHT_MARGIN 10
#define SHARE_TOP_MARGIN 25
#define SHARE_BOTTOM_MARGIN 25

#define SHARE_PROGRESS_LEFT 30
#define SHARE_PROGESS_RIGHT 30

#define SHARE_PROGRESS_SPEED 25000

// Reset after 5 seconds without a packet
#define CONN_LOST_TIMEOUT 5000000

#define SHARE_BG_COLOR c444
#define SHARE_CANVAS_BORDER c000
#define SHARE_PROGRESS_BORDER c000
#define SHARE_PROGRESS_BG c555
#define SHARE_PROGRESS_FG c350

// Uncomment to display extra connection debugging info on screen
// #define SHARE_NET_DEBUG

const uint8_t SHARE_PACKET_CANVAS_DATA = 0;
const uint8_t SHARE_PACKET_PIXEL_DATA = 1;
const uint8_t SHARE_PACKET_PIXEL_REQUEST = 2;
const uint8_t SHARE_PACKET_RECEIVE_COMPLETE = 3;
const uint8_t SHARE_PACKET_ABORT = 4;

// The canvas data packet has PAINT_MAX_COLORS bytes of palette, plus 2 uint16_ts of width/height. Also 1 for size
const uint8_t PACKET_LEN_CANVAS_DATA = sizeof(uint8_t) * PAINT_MAX_COLORS + sizeof(uint16_t) * 2;

static const char strOverwriteSlot[] = "Overwrite Slot %d";
static const char strEmptySlot[] = "Save in Slot %d";
static const char strShareSlot[] = "Share Slot %d";
static const char strControlsShare[] = "A to Share";
static const char strControlsSave[] = "A to Save";
static const char strControlsCancel[] = "B to Cancel";

paintShare_t* paintShare;

void paintShareCommonSetup(display_t* disp);
void paintShareEnterMode(display_t* disp);
void paintReceiveEnterMode(display_t* disp);
void paintShareExitMode(void);
void paintShareMainLoop(int64_t elapsedUs);
void paintShareButtonCb(buttonEvt_t* evt);
void paintShareRecvCb(const uint8_t* mac_addr, const char* data, uint8_t len, int8_t rssi);
void paintShareSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);

void paintShareP2pConnCb(p2pInfo* p2p, connectionEvt_t evt);
void paintShareP2pSendCb(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len);
void paintShareP2pMsgRecvCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len);

void paintShareRenderProgressBar(int64_t elapsedUs, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void paintRenderShareMode(int64_t elapsedUs);

void paintBeginShare(void);

void paintShareInitP2p(void);
void paintShareDeinitP2p(void);

void paintShareMsgSendOk(void);
void paintShareMsgSendFail(void);

void paintShareSendPixelRequest(void);
void paintShareSendReceiveComplete(void);
// void paintShareSendAbort(void);

void paintShareSendCanvas(void);
void paintShareHandleCanvas(void);

void paintShareSendPixels(void);
void paintShareHandlePixels(void);

void paintShareCheckForTimeout(void);
void paintShareRetry(void);

void paintShareDoLoad(void);
void paintShareDoSave(void);

#ifdef SHARE_NET_DEBUG

const char* paintShareStateToStr(paintShareState_t state);

const char* paintShareStateToStr(paintShareState_t state)
{
    switch (state)
    {
        case SHARE_SEND_SELECT_SLOT:
        return "SEL_SLOT";

        case SHARE_SEND_WAIT_FOR_CONN:
        return "S_W_CON";

        case SHARE_RECV_WAIT_FOR_CONN:
        return "R_W_CON";

        case SHARE_RECV_WAIT_CANVAS_DATA:
        return "R_W_CNV";

        case SHARE_SEND_CANVAS_DATA:
        return "S_S_CNV";

        case SHARE_SEND_WAIT_CANVAS_DATA_ACK:
        return "S_W_CNV_ACK";

        case SHARE_SEND_WAIT_FOR_PIXEL_REQUEST:
        return "S_W_PXRQ";

        case SHARE_SEND_PIXEL_DATA:
        return "S_S_PX";

        case SHARE_RECV_PIXEL_DATA:
        return "R_R_PX";

        case SHARE_SEND_WAIT_PIXEL_DATA_ACK:
        return "S_W_PX_ACK";

        case SHARE_RECV_SELECT_SLOT:
        return "SEL_SLOT";

        case SHARE_SEND_COMPLETE:
        return "DONE";

        default:
        return "?????";
    }
}

bool paintShareLogState(char* dest, size_t size);

bool paintShareLogState(char* dest, size_t size)
{
    //initialize to invalid value
    static paintShareState_t _lastState = 12;
    if (_lastState == paintShare->shareState)
    {
        return false;
    }

    snprintf(dest, size, "%s->%s", paintShareStateToStr(_lastState), paintShareStateToStr(paintShare->shareState));

    _lastState = paintShare->shareState;

    return true;
}
#endif

// Use a different swadge mode so the main game doesn't take as much battery
swadgeMode modePaintShare =
{
    .modeName = "MFPaint.net Send",
    .fnEnterMode = paintShareEnterMode,
    .fnExitMode = paintShareExitMode,
    .fnMainLoop = paintShareMainLoop,
    .fnButtonCallback = paintShareButtonCb,
    .fnTouchCallback = NULL,
    .wifiMode = ESP_NOW,
    .fnEspNowRecvCb = paintShareRecvCb,
    .fnEspNowSendCb = paintShareSendCb,
    .fnAccelerometerCallback = NULL,
    .fnAudioCallback = NULL,
    .fnTemperatureCallback = NULL,
};

swadgeMode modePaintReceive =
{
    .modeName = "MFPaint.net Recv",
    .fnEnterMode = paintReceiveEnterMode,
    .fnExitMode = paintShareExitMode,
    .fnMainLoop = paintShareMainLoop,
    .fnButtonCallback = paintShareButtonCb,
    .fnTouchCallback = NULL,
    .wifiMode = ESP_NOW,
    .fnEspNowRecvCb = paintShareRecvCb,
    .fnEspNowSendCb = paintShareSendCb,
    .fnAccelerometerCallback = NULL,
    .fnAudioCallback = NULL,
    .fnTemperatureCallback = NULL,
};

static bool isSender(void)
{
    return paintShare->isSender;
}

void paintShareInitP2p(void)
{
    paintShare->connectionStarted = true;
    paintShare->shareSeqNum = 0;
    paintShare->shareNewPacket = false;

    p2pDeinit(&paintShare->p2pInfo);
    p2pInitialize(&paintShare->p2pInfo, isSender() ? 'P' : 'Q', paintShareP2pConnCb, paintShareP2pMsgRecvCb, -35);
    p2pSetAsymmetric(&paintShare->p2pInfo, isSender() ? 'Q' : 'P');
    p2pStartConnection(&paintShare->p2pInfo);
}

void paintShareDeinitP2p(void)
{
    paintShare->connectionStarted = false;
    p2pDeinit(&paintShare->p2pInfo);

    paintShare->shareSeqNum = 0;
    paintShare->shareNewPacket = false;
}

void paintShareCommonSetup(display_t* disp)
{
    paintShare = calloc(1, sizeof(paintShare_t));
    paintShare->disp = disp;

    PAINT_LOGD("Entering Share Mode");
    paintLoadIndex(&paintShare->index);

    paintShare->connectionStarted = false;

    if (!loadFont("radiostars.font", &paintShare->toolbarFont))
    {
        PAINT_LOGE("Unable to load font!");
    }

    if (!loadWsg("arrow12.wsg", &paintShare->arrowWsg))
    {
        PAINT_LOGE("Unable to load arrow WSG!");
    }
    else
    {
        // Recolor the arrow to black
        colorReplaceWsg(&paintShare->arrowWsg, c555, c000);
    }

    // Set the display
    paintShare->canvas.disp = paintShare->disp = disp;
    paintShare->shareNewPacket = false;
    paintShare->shareUpdateScreen = true;
    paintShare->shareTime = 0;
}

void paintReceiveEnterMode(display_t* disp)
{
    paintShareCommonSetup(disp);
    paintShare->isSender = false;

    PAINT_LOGD("Receiver: Waiting for connection");
    paintShare->shareState = SHARE_RECV_WAIT_FOR_CONN;
}

void paintShareEnterMode(display_t* disp)
{
    paintShareCommonSetup(disp);
    paintShare->isSender = true;

    //////// Load an image...

    PAINT_LOGD("Sender: Selecting slot");
    paintShare->shareState = SHARE_SEND_SELECT_SLOT;

    if (!paintGetAnySlotInUse(paintShare->index))
    {
        PAINT_LOGE("Share mode started without any saved images. Exiting");
        switchToSwadgeMode(&modePaint);
        return;
    }

    // Start on the most recently saved slot
    paintShare->shareSaveSlot = paintGetRecentSlot(paintShare->index);
    if (paintShare->shareSaveSlot == PAINT_SAVE_SLOTS)
    {
        // If there was no recently saved slot, find the first slot in use instead
        paintShare->shareSaveSlot = paintGetNextSlotInUse(paintShare->index, PAINT_SAVE_SLOTS - 1);
    }

    PAINT_LOGD("paintShare->shareSaveSlot = %d", paintShare->shareSaveSlot);

    paintShare->clearScreen = true;
}

void paintShareSendCb(const uint8_t* mac_addr, esp_now_send_status_t status)
{
    p2pSendCb(&paintShare->p2pInfo, mac_addr, status);
}

void paintShareRenderProgressBar(int64_t elapsedUs, uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
    // okay, we're gonna have a real progress bar, not one of those lying fake progress bars
    // 1. While waiting to connect, draw an indeterminate progress bar, like [||  ||  ||  ||] -> [ ||  ||  ||  |] -> [  ||  ||  ||  ]
    // 2. Once connected, we use an absolute progress bar. Basically it's out of the total number of bytes
    // 3. Canvas data: Ok, we'll treat each packet as though it's the same size. After sending, we add 2 to the progress. After ACK, we add 1. After pixel data request, we add another (or is that just the next one)
    // 4. Pixel data: 2 for send, +1 for ack, +1 for receiving

    bool indeterminate = false;
    uint16_t progress = 0;

    switch (paintShare->shareState)
    {
        case SHARE_SEND_SELECT_SLOT:
        // No draw
        return;

        case SHARE_SEND_WAIT_FOR_CONN:
        case SHARE_RECV_WAIT_FOR_CONN:
        case SHARE_RECV_WAIT_CANVAS_DATA:
        // We're not connected yet, or don't yet know how much data to expect
        indeterminate = true;
        break;

        case SHARE_SEND_CANVAS_DATA:
        // Haven't sent anything yet, progress at 0
        progress = 0;
        break;

        case SHARE_SEND_WAIT_CANVAS_DATA_ACK:
        // Sent the canvas data, progress at 2
        progress = 2;
        break;

        case SHARE_SEND_WAIT_FOR_PIXEL_REQUEST:
        // Sent (canvas or pixel data, depending on seqnum), progress at 4*(seqnum+1) + 3
        progress = 4 * (paintShare->shareSeqNum + 1) + 3;
        break;

        case SHARE_SEND_PIXEL_DATA:
        case SHARE_RECV_PIXEL_DATA:
        // WWaiting to send or receive pixel data, progress at 4*(seqnum+1) + 0
        progress = 4 * (paintShare->shareSeqNum + 1);
        break;

        case SHARE_SEND_WAIT_PIXEL_DATA_ACK:
        // Sent pixel data, progress at 4*(seqnum+1) + 2
        progress = 4 * (paintShare->shareSeqNum + 1) + 2;
        break;

        case SHARE_RECV_SELECT_SLOT:
        case SHARE_SEND_COMPLETE:
        // We're done, draw the whole progress bar for fun
        progress = 0xFFFF;
        break;
    }

    // Draw border
    plotRect(paintShare->disp, x, y, x + w, y + h, SHARE_PROGRESS_BORDER);
    plotRectFilled(paintShare->disp, x + 1, y + 1, x + w, y + h, SHARE_PROGRESS_BG);

    if (!indeterminate)
    {
        paintShare->shareTime = 0;
        // if we have the canvas dimensinos, we can calculate the max progress
        // (WIDTH / ((totalPacketNum + 1) * 4)) * (currentPacketNum) * 4 + (sent: 2, acked: 3, req'd: 4)
        uint16_t maxProgress = ((paintShare->canvas.h * paintShare->canvas.w + PAINT_SHARE_PX_PER_PACKET - 1) / PAINT_SHARE_PX_PER_PACKET + 1);

        // Now, we just draw a box at (progress * (width) / maxProgress)
        uint16_t size = (progress > maxProgress ? maxProgress : progress) * w / maxProgress;
        plotRectFilled(paintShare->disp, x + 1, y + 1, x + size, y + h, SHARE_PROGRESS_FG);
    }
    else
    {
        uint8_t segCount = 4;
        uint8_t segW = ((w - 2) / segCount / 2);
        uint8_t offset = (elapsedUs / SHARE_PROGRESS_SPEED) % ((w - 2) / segCount);

        for (uint8_t i = 0; i < segCount; i++)
        {
            uint16_t x0 = (offset + i * (segW * 2)) % (w - 2);
            uint16_t x1 = (offset + i * (segW * 2) + segW) % (w - 2);

            if (x0 >= x1)
            {
                // Split the segment into two parts
                // From x0 to MAX
                plotRectFilled(paintShare->disp, x + 1 + x0, y + 1, x + w, y + h, SHARE_PROGRESS_FG);

                if (x1 != 0)
                {
                    // From 0 to x1
                    // Don't draw this if x1 == 0, because then x + 1 == x + 1 + x1, and there would be no box
                    plotRectFilled(paintShare->disp, x + 1, y + 1, x + 1 + x1, y + h, SHARE_PROGRESS_FG);
                }
            }
            else
            {
                plotRectFilled(paintShare->disp, x + 1 + x0, y + 1, x + 1 + x1, y + h, SHARE_PROGRESS_FG);
            }
        }
    }
}

void paintRenderShareMode(int64_t elapsedUs)
{

    if (paintShare->canvas.h != 0 && paintShare->canvas.w != 0)
    {
        // Top part of screen
        fillDisplayArea(paintShare->disp, 0, 0, paintShare->disp->w, paintShare->canvas.y, SHARE_BG_COLOR);

        // Left side of screen
        fillDisplayArea(paintShare->disp, 0, 0, paintShare->canvas.x, paintShare->disp->h, SHARE_BG_COLOR);

        // Right side of screen
        fillDisplayArea(paintShare->disp, paintShare->canvas.x + paintShare->canvas.w * paintShare->canvas.xScale, 0, paintShare->disp->w, paintShare->disp->h, SHARE_BG_COLOR);

        // Bottom of screen
        fillDisplayArea(paintShare->disp, 0, paintShare->canvas.y + paintShare->canvas.h * paintShare->canvas.yScale, paintShare->disp->w, paintShare->disp->h, SHARE_BG_COLOR);

        // Border the canvas
        plotRect(paintShare->disp, paintShare->canvas.x - 1, paintShare->canvas.y - 1, paintShare->canvas.x + paintShare->canvas.w * paintShare->canvas.xScale + 1, paintShare->canvas.y + paintShare->canvas.h * paintShare->canvas.yScale + 1, SHARE_CANVAS_BORDER);
    }
    else
    {
        // There's no canvas, so just... clear everything
        fillDisplayArea(paintShare->disp, 0, 0, paintShare->disp->w, paintShare->disp->h, SHARE_BG_COLOR);
    }

    char text[32];
    const char *bottomText = NULL;
    bool arrows = false;

    switch (paintShare->shareState)
    {
        case SHARE_RECV_SELECT_SLOT:
        {
            arrows = true;
            bottomText = strControlsSave;
            snprintf(text, sizeof(text), paintGetSlotInUse(paintShare->index, paintShare->shareSaveSlot) ? strOverwriteSlot : strEmptySlot, paintShare->shareSaveSlot + 1);
            break;
        }
        case SHARE_SEND_SELECT_SLOT:
        {
            arrows = true;
            bottomText = strControlsShare;
            snprintf(text, sizeof(text), strShareSlot, paintShare->shareSaveSlot + 1);
            break;
        }
        case SHARE_SEND_WAIT_FOR_CONN:
        case SHARE_RECV_WAIT_FOR_CONN:
        {
            bottomText = strControlsCancel;
            snprintf(text, sizeof(text), "Connecting...");
            break;
        }

        case SHARE_SEND_CANVAS_DATA:
        case SHARE_SEND_WAIT_CANVAS_DATA_ACK:
        case SHARE_SEND_WAIT_FOR_PIXEL_REQUEST:
        case SHARE_SEND_PIXEL_DATA:
        case SHARE_SEND_WAIT_PIXEL_DATA_ACK:
        {
            snprintf(text, sizeof(text), "Sending...");
            break;
        }

        case SHARE_SEND_COMPLETE:
        {
            snprintf(text, sizeof(text), "Complete");
            break;
        }

        case SHARE_RECV_WAIT_CANVAS_DATA:
        case SHARE_RECV_PIXEL_DATA:
        {
            snprintf(text, sizeof(text), "Receiving...");
            break;
        }
    }

#ifdef SHARE_NET_DEBUG
    static char debugText[32] = {{0}};
    paintShareLogState(debugText, sizeof(debugText));
    bottomText = debugText;
#endif

    // debug lines
    //plotLine(paintShare->disp, 0, SHARE_TOP_MARGIN, paintShare->disp->w, SHARE_TOP_MARGIN, c000, 2);
    //plotLine(paintShare->disp, 0, paintShare->disp->h - SHARE_BOTTOM_MARGIN, paintShare->disp->w, paintShare->disp->h - SHARE_BOTTOM_MARGIN, c000, 2);

    paintShareRenderProgressBar(elapsedUs, SHARE_PROGRESS_LEFT, 0, paintShare->disp->w - SHARE_PROGESS_RIGHT - SHARE_PROGRESS_LEFT, SHARE_TOP_MARGIN);

    // Draw the text over the progress bar
    uint16_t w = textWidth(&paintShare->toolbarFont, text);
    uint16_t y = (SHARE_TOP_MARGIN - paintShare->toolbarFont.h) / 2;
    drawText(paintShare->disp, &paintShare->toolbarFont, c000, text, (paintShare->disp->w - w) / 2, y);
    if (arrows)
    {
        // flip instead of using rotation to prevent 1px offset
        drawWsg(paintShare->disp, &paintShare->arrowWsg, (paintShare->disp->w - w) / 2 - paintShare->arrowWsg.w - 6, y + (paintShare->toolbarFont.h - paintShare->arrowWsg.h) / 2, false, true, 90);
        drawWsg(paintShare->disp, &paintShare->arrowWsg, (paintShare->disp->w - w) / 2 + w + 6, y + (paintShare->toolbarFont.h - paintShare->arrowWsg.h) / 2, false, false, 90);
    }

    if (bottomText != NULL)
    {
        if (paintShare->canvas.h > 0 && paintShare->canvas.yScale > 0)
        {
            y = paintShare->canvas.y + paintShare->canvas.h * paintShare->canvas.yScale + (paintShare->disp->h - paintShare->canvas.y - paintShare->canvas.h * paintShare->canvas.yScale - paintShare->toolbarFont.h) / 2;
        }
        else
        {
            y = paintShare->disp->h - paintShare->toolbarFont.h - 8;
        }

        w = textWidth(&paintShare->toolbarFont, bottomText);
        drawText(paintShare->disp, &paintShare->toolbarFont, c000, bottomText, (paintShare->disp->w - w) / 2, y);
    }
}

void paintShareSendCanvas(void)
{
    PAINT_LOGI("Sending canvas metadata...");
    // Set the length to the canvas data packet length, plus one for the packet type
    paintShare->sharePacketLen = PACKET_LEN_CANVAS_DATA + 1;
    paintShare->sharePacket[0] = SHARE_PACKET_CANVAS_DATA;

    for (uint8_t i = 0; i < PAINT_MAX_COLORS; i++)
    {
        paintShare->sharePacket[i + 1] = paintShare->canvas.palette[i];
    }

    // pack the canvas dimensions in big-endian
    // Height MSB
    paintShare->sharePacket[PAINT_MAX_COLORS + 1] = ((uint8_t)((paintShare->canvas.h >> 8) & 0xFF));
    // Height LSB
    paintShare->sharePacket[PAINT_MAX_COLORS + 2] = ((uint8_t)((paintShare->canvas.h >> 0) & 0xFF));
    // Width MSB
    paintShare->sharePacket[PAINT_MAX_COLORS + 3] = ((uint8_t)((paintShare->canvas.w >> 8) & 0xFF));
    // Height LSB
    paintShare->sharePacket[PAINT_MAX_COLORS + 4] = ((uint8_t)((paintShare->canvas.w >> 0) & 0xFF));

    paintShare->shareState = SHARE_SEND_WAIT_CANVAS_DATA_ACK;
    paintShare->shareNewPacket = false;

    p2pSendMsg(&paintShare->p2pInfo, paintShare->sharePacket, paintShare->sharePacketLen, paintShareP2pSendCb);
}

void paintShareHandleCanvas(void)
{
    PAINT_LOGD("Handling %d bytes of canvas data", paintShare->sharePacketLen);
    paintShare->shareNewPacket = false;

    if (paintShare->sharePacket[0] != SHARE_PACKET_CANVAS_DATA)
    {
        PAINT_LOGE("Canvas data has wrong type %d!!!", paintShare->sharePacket[0]);
        return;
    }

    for (uint8_t i = 0; i < PAINT_MAX_COLORS; i++)
    {
        paintShare->canvas.palette[i] = paintShare->sharePacket[i + 1];
        PAINT_LOGD("paletteMap[%d] = %d", paintShare->canvas.palette[i], i);
    }

    paintShare->canvas.h = (paintShare->sharePacket[PAINT_MAX_COLORS + 1] << 8) | (paintShare->sharePacket[PAINT_MAX_COLORS + 2]);
    paintShare->canvas.w = (paintShare->sharePacket[PAINT_MAX_COLORS + 3] << 8) | (paintShare->sharePacket[PAINT_MAX_COLORS + 4]);

    PAINT_LOGD("Canvas dimensions: %d x %d", paintShare->canvas.w, paintShare->canvas.h);

    uint8_t scale = paintGetMaxScale(paintShare->canvas.disp, paintShare->canvas.w, paintShare->canvas.h, SHARE_LEFT_MARGIN + SHARE_RIGHT_MARGIN, SHARE_TOP_MARGIN + SHARE_BOTTOM_MARGIN);
    paintShare->canvas.xScale = scale;
    paintShare->canvas.yScale = scale;

    paintShare->canvas.x = SHARE_LEFT_MARGIN + (paintShare->canvas.disp->w - SHARE_LEFT_MARGIN - SHARE_RIGHT_MARGIN - paintShare->canvas.w * paintShare->canvas.xScale) / 2;
    paintShare->canvas.y = SHARE_TOP_MARGIN + (paintShare->canvas.disp->h - SHARE_TOP_MARGIN - SHARE_BOTTOM_MARGIN - paintShare->canvas.h * paintShare->canvas.yScale) / 2;

    paintShare->disp->clearPx();
    plotRectFilledScaled(paintShare->disp, 0, 0, paintShare->canvas.w, paintShare->canvas.h, c555, paintShare->canvas.x, paintShare->canvas.y, paintShare->canvas.xScale, paintShare->canvas.yScale);

    paintShare->shareState = SHARE_RECV_PIXEL_DATA;
    paintShareSendPixelRequest();
}

void paintShareSendPixels(void)
{
    paintShare->sharePacketLen = PAINT_SHARE_PX_PACKET_LEN + 3;
    // Packet type header
    paintShare->sharePacket[0] = SHARE_PACKET_PIXEL_DATA;

    // Packet seqnum
    paintShare->sharePacket[1] = (uint8_t)((paintShare->shareSeqNum >> 8) & 0xFF);
    paintShare->sharePacket[2] = (uint8_t)((paintShare->shareSeqNum >> 0) & 0xFF);

    for (uint8_t i = 0; i < PAINT_SHARE_PX_PACKET_LEN; i++)
    {
        if (PAINT_SHARE_PX_PER_PACKET * paintShare->shareSeqNum + i * 2 >= (paintShare->canvas.w * paintShare->canvas.h))
        {
            PAINT_LOGD("Breaking on last packet because %d * %d + %d * 2 >= %d * %d ---> %d >= %d", PAINT_SHARE_PX_PER_PACKET, paintShare->shareSeqNum, i, paintShare->canvas.w, paintShare->canvas.h, PAINT_SHARE_PX_PER_PACKET * paintShare->shareSeqNum + i * 2, paintShare->canvas.w * paintShare->canvas.h);
            paintShare->sharePacketLen = i + 3;
            break;
        }
        // TODO dedupe this and the nvs functions into a paintSerialize() or something
        uint16_t x0 = paintShare->canvas.x + ((PAINT_SHARE_PX_PER_PACKET * paintShare->shareSeqNum) + (i * 2)) % paintShare->canvas.w * paintShare->canvas.xScale;
        uint16_t y0 = paintShare->canvas.y + ((PAINT_SHARE_PX_PER_PACKET * paintShare->shareSeqNum) + (i * 2)) / paintShare->canvas.w * paintShare->canvas.yScale;
        uint16_t x1 = paintShare->canvas.x + ((PAINT_SHARE_PX_PER_PACKET * paintShare->shareSeqNum) + (i * 2 + 1)) % paintShare->canvas.w * paintShare->canvas.xScale;
        uint16_t y1 = paintShare->canvas.y + ((PAINT_SHARE_PX_PER_PACKET * paintShare->shareSeqNum) + (i * 2 + 1)) / paintShare->canvas.w * paintShare->canvas.yScale;

        //PAINT_LOGD("Mapping px(%d, %d) (%d) --> %x", x0, y0, paintShare->disp->getPx(x0, y0), paintShare->sharePaletteMap[(uint8_t)(paintShare->disp->getPx(x0, y0))]);
        paintShare->sharePacket[i + 3] = paintShare->sharePaletteMap[(uint8_t)paintShare->disp->getPx(x0, y0)] << 4 | paintShare->sharePaletteMap[(uint8_t)paintShare->disp->getPx(x1, y1)];
    }

    paintShare->shareState = SHARE_SEND_WAIT_PIXEL_DATA_ACK;
    PAINT_LOGD("p2pSendMsg(%p, %d)", paintShare->sharePacket, paintShare->sharePacketLen);
    PAINT_LOGD("SENDING DATA:");
    for (uint8_t i = 0; i < paintShare->sharePacketLen; i+= 4)
    {
        PAINT_LOGD("%04d %02x %02x %02x %02x", i, paintShare->sharePacket[i], paintShare->sharePacket[i+1], paintShare->sharePacket[i+2], paintShare->sharePacket[i+3]);
    }

    p2pSendMsg(&paintShare->p2pInfo, paintShare->sharePacket, paintShare->sharePacketLen, paintShareP2pSendCb);
}

void paintShareHandlePixels(void)
{
    PAINT_LOGD("Handling %d bytes of pixel data", paintShare->sharePacketLen);
    paintShare->shareNewPacket = false;

    if (paintShare->sharePacket[0] != ((uint8_t)SHARE_PACKET_PIXEL_DATA))
    {
        PAINT_LOGE("Received pixel data with incorrect type %d", paintShare->sharePacket[0]);
        PAINT_LOGE("First 16 bytes: %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x", paintShare->sharePacket[0], paintShare->sharePacket[1], paintShare->sharePacket[2], paintShare->sharePacket[3], paintShare->sharePacket[4], paintShare->sharePacket[5], paintShare->sharePacket[6], paintShare->sharePacket[7], paintShare->sharePacket[8], paintShare->sharePacket[9], paintShare->sharePacket[10], paintShare->sharePacket[11], paintShare->sharePacket[12], paintShare->sharePacket[13], paintShare->sharePacket[14], paintShare->sharePacket[15]);
        return;
    }

    paintShare->shareSeqNum = (paintShare->sharePacket[1] << 8) | paintShare->sharePacket[2];

    PAINT_LOGD("Packet seqnum is %d (%x << 8 | %x)", paintShare->shareSeqNum, paintShare->sharePacket[1], paintShare->sharePacket[2]);

    for (uint8_t i = 0; i < paintShare->sharePacketLen - 3; i++)
    {
        uint16_t x0 = ((PAINT_SHARE_PX_PER_PACKET * paintShare->shareSeqNum) + (i * 2)) % paintShare->canvas.w;
        uint16_t y0 = ((PAINT_SHARE_PX_PER_PACKET * paintShare->shareSeqNum) + (i * 2)) / paintShare->canvas.w;
        uint16_t x1 = ((PAINT_SHARE_PX_PER_PACKET * paintShare->shareSeqNum) + (i * 2 + 1)) % paintShare->canvas.w;
        uint16_t y1 = ((PAINT_SHARE_PX_PER_PACKET * paintShare->shareSeqNum) + (i * 2 + 1)) / paintShare->canvas.w;

        setPxScaled(paintShare->disp, x0, y0, paintShare->canvas.palette[paintShare->sharePacket[i + 3] >> 4], paintShare->canvas.x, paintShare->canvas.y, paintShare->canvas.xScale, paintShare->canvas.yScale);
        setPxScaled(paintShare->disp, x1, y1, paintShare->canvas.palette[paintShare->sharePacket[i + 3] & 0xF], paintShare->canvas.x, paintShare->canvas.y, paintShare->canvas.xScale, paintShare->canvas.yScale);
    }

    PAINT_LOGD("We've received %d / %d pixels", paintShare->shareSeqNum * PAINT_SHARE_PX_PER_PACKET + (paintShare->sharePacketLen - 3) * 2, paintShare->canvas.h * paintShare->canvas.w);

    if (paintShare->shareSeqNum * PAINT_SHARE_PX_PER_PACKET + (paintShare->sharePacketLen - 3) * 2 >= paintShare->canvas.h * paintShare->canvas.w)
    {
        PAINT_LOGD("I think we're done receiving");
        // We don't reeeally care if the sender acks this packet.
        // I mean, it would be polite to make sure it gets there, but there's not really a point
        paintShare->shareState = SHARE_RECV_SELECT_SLOT;
        paintShareSendReceiveComplete();
    }
    else
    {
        PAINT_LOGD("Done handling pixel packet, may we please have some more?");
        paintShareSendPixelRequest();
    }
}

void paintShareSendPixelRequest(void)
{
    paintShare->sharePacket[0] = SHARE_PACKET_PIXEL_REQUEST;
    paintShare->sharePacketLen = 1;
    p2pSendMsg(&paintShare->p2pInfo, paintShare->sharePacket, paintShare->sharePacketLen, paintShareP2pSendCb);
    paintShare->shareUpdateScreen = true;
}

void paintShareSendReceiveComplete(void)
{
    paintShare->sharePacket[0] = SHARE_PACKET_RECEIVE_COMPLETE;
    paintShare->sharePacketLen = 1;
    p2pSendMsg(&paintShare->p2pInfo, paintShare->sharePacket, paintShare->sharePacketLen, paintShareP2pSendCb);
    paintShare->shareUpdateScreen = true;
}

// void paintShareSendAbort(void)
// {
//     paintShare->sharePacket[0] = SHARE_PACKET_ABORT;
//     paintShare->sharePacketLen = 1;
//     p2pSendMsg(&paintShare->p2pInfo, paintShare->sharePacket, paintShare->sharePacketLen, paintShareP2pSendCb);
//     paintShare->shareUpdateScreen = true;
// }

void paintShareMsgSendOk(void)
{
    paintShare->shareUpdateScreen = true;
    switch (paintShare->shareState)
    {
        case SHARE_SEND_SELECT_SLOT:
        case SHARE_SEND_WAIT_FOR_CONN:
        case SHARE_SEND_CANVAS_DATA:
        break;

        case SHARE_SEND_WAIT_CANVAS_DATA_ACK:
        {
            PAINT_LOGD("Got ACK for canvas data!");
            paintShare->shareState = SHARE_SEND_WAIT_FOR_PIXEL_REQUEST;
            break;
        }

        case SHARE_SEND_WAIT_FOR_PIXEL_REQUEST:
        break;

        case SHARE_SEND_PIXEL_DATA:
        break;

        case SHARE_SEND_WAIT_PIXEL_DATA_ACK:
        {
            PAINT_LOGD("Got ACK for pixel data packet %d", paintShare->shareSeqNum);

            if (PAINT_SHARE_PX_PER_PACKET * (paintShare->shareSeqNum + 1) >= (paintShare->canvas.w * paintShare->canvas.h))
            {
                PAINT_LOGD("Probably done sending! But waiting for confirmation...");
            }
            paintShare->shareState = SHARE_SEND_WAIT_FOR_PIXEL_REQUEST;
            paintShare->shareSeqNum++;
            break;
        }

        case SHARE_SEND_COMPLETE:
        {

            break;
        }



        case SHARE_RECV_WAIT_FOR_CONN:
        {
            break;
        }

        case SHARE_RECV_WAIT_CANVAS_DATA:
        {
            break;
        }

        case SHARE_RECV_PIXEL_DATA:
        {
            break;
        }

        case SHARE_RECV_SELECT_SLOT:
        {
            break;
        }
    }
}

void paintShareMsgSendFail(void)
{
    paintShareRetry();
}


void paintBeginShare(void)
{
    paintShareInitP2p();
    paintShare->shareState = SHARE_SEND_WAIT_FOR_CONN;

    paintShare->shareSeqNum = 0;

    PAINT_LOGD("Sender: Waiting for connection...");
}

void paintShareExitMode(void)
{
    p2pDeinit(&paintShare->p2pInfo);
    freeFont(&paintShare->toolbarFont);
    freeWsg(&paintShare->arrowWsg);

    free(paintShare);

    paintShare = NULL;
}

void paintShareCheckForTimeout(void)
{
    if (paintShare->timeSincePacket >= CONN_LOST_TIMEOUT)
    {
        paintShare->timeSincePacket = 0;
        PAINT_LOGD("Conn loss detected, resetting");
        paintShare->shareState = isSender() ? SHARE_SEND_WAIT_FOR_CONN : SHARE_RECV_WAIT_FOR_CONN;
        paintShareInitP2p();
    }
}

// Go back to the previous state so we retry the last thing
void paintShareRetry(void)
{
    PAINT_LOGE("Retrying something!");
    // is that all?
    switch (paintShare->shareState)
    {
        case SHARE_SEND_SELECT_SLOT:
        case SHARE_SEND_WAIT_FOR_CONN:
        break;

        case SHARE_SEND_CANVAS_DATA:
        {
            paintShare->shareNewPacket = true;
            break;
        }

        case SHARE_SEND_WAIT_FOR_PIXEL_REQUEST:
        break;

        case SHARE_SEND_WAIT_CANVAS_DATA_ACK:
        {
            paintShare->shareState = SHARE_SEND_CANVAS_DATA;
            paintShare->shareNewPacket = true;
            break;
        }

        case SHARE_SEND_PIXEL_DATA:
        {
            paintShare->shareNewPacket = true;
            break;
        }

        case SHARE_SEND_WAIT_PIXEL_DATA_ACK:
        {
            paintShare->shareState = SHARE_SEND_PIXEL_DATA;
            paintShare->shareNewPacket = true;
            break;
        }

        case SHARE_SEND_COMPLETE:
        break;



        case SHARE_RECV_WAIT_FOR_CONN:
        case SHARE_RECV_WAIT_CANVAS_DATA:
        case SHARE_RECV_PIXEL_DATA:
        case SHARE_RECV_SELECT_SLOT:
        break;
    }
    paintShare->shareNewPacket = true;
}

void paintShareMainLoop(int64_t elapsedUs)
{
    // Handle the sending of the packets and the other things
    if (paintShare->clearScreen)
    {
        PAINT_LOGD("Redrawing!!!");
        paintShareDoLoad();
        paintShare->clearScreen = false;
    }

    paintShare->shareTime += elapsedUs;
    if (paintShare->shareNewPacket)
    {
        paintShare->timeSincePacket = 0;
    }
    else if (paintShare->shareState != SHARE_SEND_SELECT_SLOT &&
             paintShare->shareState != SHARE_RECV_SELECT_SLOT &&
             paintShare->shareState != SHARE_SEND_WAIT_FOR_CONN &&
             paintShare->shareState != SHARE_RECV_WAIT_FOR_CONN)
    {
        paintShare->timeSincePacket += elapsedUs;
    }

    switch (paintShare->shareState)
    {
        case SHARE_SEND_SELECT_SLOT:
        break;

        case SHARE_SEND_WAIT_FOR_CONN:
        paintShare->shareUpdateScreen = true;
        if (!paintShare->connectionStarted)
        {
            paintSetRecentSlot(&paintShare->index, paintShare->shareSaveSlot);
            paintShareInitP2p();
        }
        break;

        case SHARE_SEND_CANVAS_DATA:
        {
            if (paintShare->shareNewPacket)
            {
                paintShareSendCanvas();
            }
            break;
        }

        case SHARE_SEND_WAIT_CANVAS_DATA_ACK:
        {
            paintShareCheckForTimeout();
            break;
        }

        case SHARE_SEND_WAIT_FOR_PIXEL_REQUEST:
        {
            if (paintShare->shareNewPacket)
            {
                if (paintShare->sharePacket[0] == SHARE_PACKET_PIXEL_REQUEST)
                {
                    paintShare->shareNewPacket = false;
                    paintShare->shareState = SHARE_SEND_PIXEL_DATA;
                }
                else if (paintShare->sharePacket[0] == SHARE_PACKET_RECEIVE_COMPLETE)
                {
                    PAINT_LOGD("We've received confirmation! All data was received successfully");
                    paintShare->shareNewPacket = false;
                    paintShare->shareState = SHARE_SEND_COMPLETE;
                    paintShare->shareUpdateScreen = true;
                }
            }
            else
            {
                paintShareCheckForTimeout();
            }
            break;
        }

        case SHARE_SEND_PIXEL_DATA:
        {
            paintShareSendPixels();
            break;
        }

        case SHARE_SEND_WAIT_PIXEL_DATA_ACK:
        {
            // Don't need to check for timeout! p2pConnection will handle it for acks
            break;
        }

        case SHARE_SEND_COMPLETE:
        {
            paintShareDeinitP2p();
            break;
        }

        case SHARE_RECV_WAIT_FOR_CONN:
        {
            paintShare->shareUpdateScreen = true;
            if (!paintShare->connectionStarted)
            {
                PAINT_LOGD("Reiniting p2p...");
                paintShareInitP2p();
            }
            break;
        }

        case SHARE_RECV_WAIT_CANVAS_DATA:
        {
            if (paintShare->shareNewPacket)
            {
                paintShareHandleCanvas();
            }
            else
            {
                paintShareCheckForTimeout();
            }
            paintShare->shareUpdateScreen = true;

            break;
        }

        case SHARE_RECV_PIXEL_DATA:
        {
            if (paintShare->shareNewPacket)
            {
                paintShareHandlePixels();
            }
            else
            {
                paintShareCheckForTimeout();
            }
            paintShare->shareUpdateScreen = true;
            break;
        }

        case SHARE_RECV_SELECT_SLOT:
        {
            paintShareDeinitP2p();
            break;
        }
    }

    if (paintShare->shareUpdateScreen)
    {
        paintRenderShareMode(paintShare->shareTime);
        paintShare->shareUpdateScreen = false;
    }
}

void paintShareButtonCb(buttonEvt_t* evt)
{
    if (paintShare->shareState == SHARE_SEND_SELECT_SLOT)
    {
        if (evt->down)
        {
            switch (evt->button)
            {
                case LEFT:
                {
                    // Load previous image
                    paintShare->shareSaveSlot = paintGetPrevSlotInUse(paintShare->index, paintShare->shareSaveSlot);
                    paintShare->clearScreen = true;
                    paintShare->shareUpdateScreen = true;
                    break;
                }
                case RIGHT:
                {
                    // Load next image
                    paintShare->shareSaveSlot = paintGetNextSlotInUse(paintShare->index, paintShare->shareSaveSlot);
                    paintShare->clearScreen = true;
                    paintShare->shareUpdateScreen = true;
                    break;
                }

                case BTN_A:
                {
                    // Begin sharing!
                    paintBeginShare();
                    paintShare->shareUpdateScreen = true;
                    break;
                }

                case BTN_B:
                {
                    switchToSwadgeMode(&modePaint);
                    break;
                }

                case UP:
                case DOWN:
                // Do Nothing!
                case SELECT:
                case START:
                // Or do something on button up to avoid conflict with exit mode
                break;
            }
        }
        else
        {
            if (evt->button == SELECT)
            {
                paintShare->shareSaveSlot = paintGetNextSlotInUse(paintShare->index, paintShare->shareSaveSlot);
                paintShare->clearScreen = true;
                paintShare->shareUpdateScreen = true;
            }
            else if (evt->button == START)
            {
                paintBeginShare();
                paintShare->shareUpdateScreen = true;
            }
        }
    }
    else if (paintShare->shareState == SHARE_RECV_SELECT_SLOT)
    {
        if (evt->down)
        {
            switch (evt->button)
            {
                case LEFT:
                {
                    paintShare->shareSaveSlot = PREV_WRAP(paintShare->shareSaveSlot, PAINT_SAVE_SLOTS);
                    paintShare->shareUpdateScreen = true;
                    break;
                }

                case RIGHT:
                {
                    paintShare->shareSaveSlot = NEXT_WRAP(paintShare->shareSaveSlot, PAINT_SAVE_SLOTS);
                    paintShare->shareUpdateScreen = true;
                    break;
                }

                case BTN_A:
                {
                    paintShareDoSave();
                    switchToSwadgeMode(&modePaint);
                    break;
                }

                case BTN_B:
                {
                    // Exit without saving
                    switchToSwadgeMode(&modePaint);
                    break;
                }

                case UP:
                case DOWN:
                // Do Nothing!
                case SELECT:
                case START:
                // Or do something on button-up instead, to avoid overlap with SELECT+START
                break;
            }
        }
        else
        {
            if (evt->button == START)
            {
                paintShareDoSave();
                switchToSwadgeMode(&modePaint);
            }
            else if (evt->button == SELECT)
            {
                paintShare->shareSaveSlot = NEXT_WRAP(paintShare->shareSaveSlot, PAINT_SAVE_SLOTS);
                paintShare->shareUpdateScreen = true;
            }
        }
        // Does the receiver get any buttons?
        // Yes! They need to pick their destination slot before starting P2P
    } else if (paintShare->shareState == SHARE_SEND_COMPLETE)
    {
        if (evt->down && evt->button == BTN_B)
        {
            switchToSwadgeMode(&modePaint);
        }
        else
        {
            paintShare->shareState = SHARE_SEND_SELECT_SLOT;
            paintShare->shareUpdateScreen = true;
        }
    } else if (paintShare->shareState == SHARE_SEND_WAIT_FOR_CONN || paintShare->shareState == SHARE_RECV_WAIT_FOR_CONN)
    {
        if (evt->down && evt->button == BTN_B)
        {
            switchToSwadgeMode(&modePaint);
        }
    }
}

void paintShareRecvCb(const uint8_t* mac_addr, const char* data, uint8_t len, int8_t rssi)
{
    p2pRecvCb(&paintShare->p2pInfo, mac_addr, (const uint8_t*)data, len, rssi);
}

void paintShareP2pSendCb(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len)
{
    switch (status)
    {
        case MSG_ACKED:
        PAINT_LOGD("ACK");
        paintShareMsgSendOk();
        break;

        case MSG_FAILED:
        PAINT_LOGE("FAILED!!!");
        paintShareMsgSendFail();
        break;
    }
}

void paintShareP2pConnCb(p2pInfo* p2p, connectionEvt_t evt)
{
    switch(evt)
    {
        case CON_STARTED:
        PAINT_LOGD("CON_STARTED");
        break;

        case RX_GAME_START_ACK:
        PAINT_LOGD("RX_GAME_START_ACK");
        break;

        case RX_GAME_START_MSG:
        PAINT_LOGD("RX_GAME_START_MSG");
        break;

        case CON_ESTABLISHED:
        {
            PAINT_LOGD("CON_ESTABLISHED");
            if (paintShare->shareState == SHARE_SEND_WAIT_FOR_CONN)
            {
                PAINT_LOGD("state = SHARE_SEND_CANVAS_DATA");
                paintShare->shareState = SHARE_SEND_CANVAS_DATA;
                paintShare->shareNewPacket = true;
            }
            else if (paintShare->shareState == SHARE_RECV_WAIT_FOR_CONN)
            {
                PAINT_LOGD("state = SHARE_RECV_WAIT_CANVAS_DATA");
                paintShare->shareState = SHARE_RECV_WAIT_CANVAS_DATA;
            }

            break;
        }

        case CON_LOST:
        {
            PAINT_LOGD("CON_LOST");
            paintShareInitP2p();

            // We don't want to time out while waiting for a connection
            paintShare->timeSincePacket = 0;
            if (isSender())
            {
                paintShare->shareState = SHARE_SEND_WAIT_FOR_CONN;
            }
            else
            {
                paintShare->shareState = SHARE_RECV_WAIT_FOR_CONN;
            }

            break;
        }
    }
}

void paintShareP2pMsgRecvCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len)
{
    // no buffer overruns for me thanks
    PAINT_LOGV("Receiving %d bytes via P2P callback", len);
    memcpy(paintShare->sharePacket, payload, len);

    paintShare->sharePacketLen = len;
    paintShare->shareNewPacket = true;
}

void paintShareDoLoad(void)
{
    paintShare->disp->clearPx();
    // Load just image dimensions;

    if (!paintLoadDimensions(&paintShare->canvas, paintShare->shareSaveSlot))
    {
        PAINT_LOGE("Failed to load dimensions, stopping load");
        return;
    }

    // With the image dimensions, calculate the max scale that will fit on the screen
    uint8_t scale = paintGetMaxScale(paintShare->canvas.disp, paintShare->canvas.w, paintShare->canvas.h, SHARE_LEFT_MARGIN + SHARE_RIGHT_MARGIN, SHARE_TOP_MARGIN + SHARE_BOTTOM_MARGIN);
    PAINT_LOGD("Loading image at scale %d", scale);
    paintShare->canvas.xScale = scale;
    paintShare->canvas.yScale = scale;

    // Center the canvas on the empty area of the screen
    paintShare->canvas.x = SHARE_LEFT_MARGIN + (paintShare->canvas.disp->w - SHARE_LEFT_MARGIN - SHARE_RIGHT_MARGIN - paintShare->canvas.w * paintShare->canvas.xScale) / 2;
    paintShare->canvas.y = SHARE_TOP_MARGIN + (paintShare->canvas.disp->h - SHARE_TOP_MARGIN - SHARE_BOTTOM_MARGIN - paintShare->canvas.h * paintShare->canvas.yScale) / 2;

    // Load the actual image!
    // If all goes well, it will be drawn centered and as big as possible
    paintLoad(&paintShare->index, &paintShare->canvas, paintShare->shareSaveSlot);

    for (uint8_t i = 0; i < PAINT_MAX_COLORS; i++)
    {
        paintShare->sharePaletteMap[(uint8_t)(paintShare->canvas.palette[i])] = i;
    }
}

void paintShareDoSave(void)
{
    paintSave(&paintShare->index, &paintShare->canvas, paintShare->shareSaveSlot);
}
