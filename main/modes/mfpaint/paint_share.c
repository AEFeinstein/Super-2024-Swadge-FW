#include "paint_share.h"

#include <stddef.h>
#include <string.h>

#include "p2pConnection.h"
#include "shapes.h"
#include "hdw-btn.h"

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
 * - Now, we begin sending packets. We wait until each one is ACKed before sending another (TODO: don't send another
 * packet until the receiver asks for more)
 * - Each packet contains an absolute sequence number and as many bytes of pixel data as will fit (palette-indexed and
 * packed into 2 pixels per byte)
 * - Once the last packet has been acked, we're done! Return to share mode
 */

#define SHARE_LEFT_MARGIN   10
#define SHARE_RIGHT_MARGIN  10
#define SHARE_TOP_MARGIN    25
#define SHARE_BOTTOM_MARGIN 25

#define SHARE_PROGRESS_LEFT 30
#define SHARE_PROGESS_RIGHT 30

#define SHARE_PROGRESS_SPEED 25000

// Reset after 5 seconds without a packet
#define CONN_LOST_TIMEOUT 5000000

#define SHARE_BG_COLOR        c444
#define SHARE_CANVAS_BORDER   c000
#define SHARE_PROGRESS_BORDER c000
#define SHARE_PROGRESS_BG     c555
#define SHARE_PROGRESS_FG     c350

// Uncomment to display extra connection debugging info on screen
//#define SHARE_NET_DEBUG

const uint8_t SHARE_PACKET_CANVAS_DATA      = 0;
const uint8_t SHARE_PACKET_PIXEL_DATA       = 1;
const uint8_t SHARE_PACKET_PIXEL_REQUEST    = 2;
const uint8_t SHARE_PACKET_RECEIVE_COMPLETE = 3;
const uint8_t SHARE_PACKET_ABORT            = 4;
// yes there is a version in the canvas data, but we also need it on the other side
const uint8_t SHARE_PACKET_VERSION          = 5;

// The canvas data packet has PAINT_MAX_COLORS bytes of palette, plus 2 uint16_ts of width/height. Also 1 for size
const uint8_t PACKET_LEN_CANVAS_DATA_V0 = sizeof(uint8_t) * PAINT_MAX_COLORS + sizeof(uint16_t) * 2;
// v1 adds another byte at the end for version
const uint8_t PACKET_LEN_CANVAS_DATA_V1 = sizeof(uint8_t) * PAINT_MAX_COLORS + sizeof(uint16_t) * 2 + sizeof(uint8_t);

static const char strControlsShare[] = "A to Share";
static const char strControlsSave[]   = "A to Save";
static const char strControlsCancel[]  = "B to Cancel";
static const char strSelectShareSlot[] = "Select Drawing to Share";
static const char strSelectSaveSlot[]  = "Select Destination";

paintShare_t* paintShare;

void paintShareCommonSetup(void);
void paintShareEnterMode(void);
void paintReceiveEnterMode(void);
void paintShareExitMode(void);
void paintShareMainLoop(int64_t elapsedUs);
void paintShareButtonCb(buttonEvt_t* evt);
void paintShareRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi);
void paintShareSendCb(const uint8_t* mac_addr, esp_now_send_status_t status);

void paintShareP2pConnCb(p2pInfo* p2p, connectionEvt_t evt);
void paintShareP2pSendCb(p2pInfo* p2p, messageStatus_t status, const uint8_t* data, uint8_t len);
void paintShareP2pMsgRecvCb(p2pInfo* p2p, const uint8_t* payload, uint8_t len);
static void paintShareBrowserCb(const char* key, imageBrowserAction_t action);

void paintShareRenderProgressBar(int64_t elapsedUs, uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void paintRenderShareMode(int64_t elapsedUs);

void paintBeginShare(void);

void paintShareInitP2p(void);
void paintShareDeinitP2p(void);

void paintShareMsgSendOk(void);
void paintShareMsgSendFail(void);

void paintShareSendVersion(void);

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
    // initialize to invalid value
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
swadgeMode_t modePaintShare = {
    .modeName                 = "MFPaint.net Send",
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = true,
    .fnEnterMode              = paintShareEnterMode,
    .fnExitMode               = paintShareExitMode,
    .fnMainLoop               = paintShareMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = paintShareRecvCb,
    .fnEspNowSendCb           = paintShareSendCb,
    .fnAdvancedUSB            = NULL,
};

swadgeMode_t modePaintReceive = {
    .modeName                 = "MFPaint.net Recv",
    .wifiMode                 = ESP_NOW,
    .overrideUsb              = false,
    .usesAccelerometer        = false,
    .usesThermometer          = false,
    .overrideSelectBtn        = true,
    .fnEnterMode              = paintReceiveEnterMode,
    .fnExitMode               = paintShareExitMode,
    .fnMainLoop               = paintShareMainLoop,
    .fnAudioCallback          = NULL,
    .fnBackgroundDrawCallback = NULL,
    .fnEspNowRecvCb           = paintShareRecvCb,
    .fnEspNowSendCb           = paintShareSendCb,
    .fnAdvancedUSB            = NULL,
};

static bool isSender(void)
{
    return paintShare->isSender;
}

void paintShareInitP2p(void)
{
    paintShare->connectionStarted = true;
    paintShare->shareSeqNum       = 0;
    paintShare->shareNewPacket    = false;

    p2pDeinit(&paintShare->p2pInfo);

    // Support using cable
    if (paintShare->useCable)
    {
        espNowUseSerial(isSender());
    }
    else
    {
        espNowUseWireless();
    }

    p2pInitialize(&paintShare->p2pInfo, isSender() ? 'P' : 'Q', paintShareP2pConnCb, paintShareP2pMsgRecvCb, -35);
    p2pSetAsymmetric(&paintShare->p2pInfo, isSender() ? 'Q' : 'P');
    p2pStartConnection(&paintShare->p2pInfo);
}

void paintShareDeinitP2p(void)
{
    paintShare->connectionStarted = false;
    p2pDeinit(&paintShare->p2pInfo);

    paintShare->shareSeqNum    = 0;
    paintShare->shareNewPacket = false;
}

void paintShareCommonSetup(void)
{
    paintShare = calloc(1, sizeof(paintShare_t));

    PAINT_LOGD("Entering Share Mode");

    paintShare->dialog = initDialogBox("hello", "test", NULL, NULL);

    paintShare->connectionStarted = false;

    if (!loadFont(PAINT_SHARE_TOOLBAR_FONT, &paintShare->toolbarFont, false))
    {
        PAINT_LOGE("Unable to load font!");
    }

    if (!loadWsg("button_up.wsg", &paintShare->arrowWsg, false))
    {
        PAINT_LOGE("Unable to load arrow WSG!");
    }

    paintShare->browser.callback   = paintShareBrowserCb;
    paintShare->browser.wraparound = true;
    paintShare->browser.cols       = 4;
    paintShare->browser.viewStyle  = BROWSER_GALLERY;

    // Set the display
    paintShare->shareNewPacket    = false;
    paintShare->shareUpdateScreen = true;
    paintShare->shareTime         = 0;
}

void paintReceiveEnterMode(void)
{
    paintShareCommonSetup();
    paintShare->isSender = false;

    paintShare->browser.title = strSelectSaveSlot;
    setupImageBrowser(&paintShare->browser, &paintShare->toolbarFont, PAINT_NS_DATA, NULL, BROWSER_SAVE, BROWSER_SAVE);
    paintShare->browserVisible = false;

    PAINT_LOGD("Receiver: Waiting for connection");
    paintShare->shareState = SHARE_RECV_WAIT_FOR_CONN;
}

void paintShareEnterMode(void)
{
    paintShareCommonSetup();
    paintShare->isSender = true;

    paintShare->browser.title = strSelectShareSlot;
    paintShare->browser.viewStyle = BROWSER_GALLERY;
    setupImageBrowser(&paintShare->browser, &paintShare->toolbarFont, PAINT_NS_DATA, NULL, BROWSER_OPEN, BROWSER_OPEN);
    paintShare->browserVisible = true;

    //////// Load an image...

    PAINT_LOGD("Sender: Selecting slot");
    paintShare->shareState = SHARE_SEND_SELECT_SLOT;

    if (!paintGetAnySlotInUse())
    {
        PAINT_LOGE("Share mode started without any saved images. Exiting");
        switchToSwadgeMode(&modePaint);
        return;
    }

    // Start on the most recently saved slot
    paintGetLastSlot(paintShare->shareSaveSlotKey);

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
    // 1. While waiting to connect, draw an indeterminate progress bar, like [||  ||  ||  ||] -> [ ||  ||  ||  |] -> [
    // ||  ||  ||  ]
    // 2. Once connected, we use an absolute progress bar. Basically it's out of the total number of bytes
    // 3. Canvas data: Ok, we'll treat each packet as though it's the same size. After sending, we add 2 to the
    // progress. After ACK, we add 1. After pixel data request, we add another (or is that just the next one)
    // 4. Pixel data: 2 for send, +1 for ack, +1 for receiving

    bool indeterminate = false;
    uint16_t progress  = 0;

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
    drawRect(x, y, x + w, y + h, SHARE_PROGRESS_BORDER);
    drawRectFilled(x + 1, y + 1, x + w, y + h, SHARE_PROGRESS_BG);

    if (!indeterminate)
    {
        paintShare->shareTime = 0;
        // if we have the canvas dimensinos, we can calculate the max progress
        // (WIDTH / ((totalPacketNum + 1) * 4)) * (currentPacketNum) * 4 + (sent: 2, acked: 3, req'd: 4)
        uint16_t maxProgress
            = ((paintShare->canvas.h * paintShare->canvas.w + PAINT_SHARE_PX_PER_PACKET - 1) / PAINT_SHARE_PX_PER_PACKET
               + 1);

        // Now, we just draw a box at (progress * (width) / maxProgress)
        uint16_t size = (progress > maxProgress ? maxProgress : progress) * w / maxProgress;
        drawRectFilled(x + 1, y + 1, x + size, y + h, SHARE_PROGRESS_FG);
    }
    else
    {
        uint8_t segCount = 4;
        uint8_t segW     = ((w - 2) / segCount / 2);
        uint8_t offset   = (elapsedUs / SHARE_PROGRESS_SPEED) % ((w - 2) / segCount);

        for (uint8_t i = 0; i < segCount; i++)
        {
            uint16_t x0 = (offset + i * (segW * 2)) % (w - 2);
            uint16_t x1 = (offset + i * (segW * 2) + segW) % (w - 2);

            if (x0 >= x1)
            {
                // Split the segment into two parts
                // From x0 to MAX
                drawRectFilled(x + 1 + x0, y + 1, x + w, y + h, SHARE_PROGRESS_FG);

                if (x1 != 0)
                {
                    // From 0 to x1
                    // Don't draw this if x1 == 0, because then x + 1 == x + 1 + x1, and there would be no box
                    drawRectFilled(x + 1, y + 1, x + 1 + x1, y + h, SHARE_PROGRESS_FG);
                }
            }
            else
            {
                drawRectFilled(x + 1 + x0, y + 1, x + 1 + x1, y + h, SHARE_PROGRESS_FG);
            }
        }
    }
}

void paintRenderShareMode(int64_t elapsedUs)
{
    if (paintShare->canvas.h != 0 && paintShare->canvas.w != 0)
    {
        // Top part of screen
        fillDisplayArea(0, 0, TFT_WIDTH, paintShare->canvas.y, SHARE_BG_COLOR);

        // Left side of screen
        fillDisplayArea(0, 0, paintShare->canvas.x, TFT_HEIGHT, SHARE_BG_COLOR);

        // Right side of screen
        fillDisplayArea(paintShare->canvas.x + paintShare->canvas.w * paintShare->canvas.xScale, 0, TFT_WIDTH,
                        TFT_HEIGHT, SHARE_BG_COLOR);

        // Bottom of screen
        fillDisplayArea(0, paintShare->canvas.y + paintShare->canvas.h * paintShare->canvas.yScale, TFT_WIDTH,
                        TFT_HEIGHT, SHARE_BG_COLOR);

        // Border the canvas
        drawRect(paintShare->canvas.x - 1, paintShare->canvas.y - 1,
                 paintShare->canvas.x + paintShare->canvas.w * paintShare->canvas.xScale + 1,
                 paintShare->canvas.y + paintShare->canvas.h * paintShare->canvas.yScale + 1, SHARE_CANVAS_BORDER);
    }
    else
    {
        // There's no canvas, so just... clear everything
        fillDisplayArea(0, 0, TFT_WIDTH, TFT_HEIGHT, SHARE_BG_COLOR);
    }

    char text[32];
    const char* bottomText = NULL;
    bool arrows            = false;

    switch (paintShare->shareState)
    {
        case SHARE_RECV_SELECT_SLOT:
        {
            if (!paintShare->browserVisible)
            {
                snprintf(text, sizeof(text), strControlsSave);
            }
            break;
        }
        case SHARE_SEND_SELECT_SLOT:
        {
            bottomText = strControlsShare;
            snprintf(text, sizeof(text), "%s", paintShare->shareSaveSlotKey);
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
    static char debugText[32] = {0};
    paintShareLogState(debugText, sizeof(debugText));
    bottomText = debugText;
#endif

    // debug lines
    // drawLine(paintShare->disp, 0, SHARE_TOP_MARGIN, paintShare->disp->w, SHARE_TOP_MARGIN, c000, 2);
    // drawLine(paintShare->disp, 0, paintShare->disp->h - SHARE_BOTTOM_MARGIN, paintShare->disp->w, paintShare->disp->h
    // - SHARE_BOTTOM_MARGIN, c000, 2);

    paintShareRenderProgressBar(elapsedUs, SHARE_PROGRESS_LEFT, 0,
                                TFT_WIDTH - SHARE_PROGESS_RIGHT - SHARE_PROGRESS_LEFT, SHARE_TOP_MARGIN);

    // Draw the text over the progress bar
    uint16_t w = textWidth(&paintShare->toolbarFont, text);
    uint16_t y = (SHARE_TOP_MARGIN - paintShare->toolbarFont.height) / 2;
    drawText(&paintShare->toolbarFont, c000, text, (TFT_WIDTH - w) / 2, y);
    if (arrows)
    {
        // flip instead of using rotation to prevent 1px offset
        drawWsg(&paintShare->arrowWsg, (TFT_WIDTH - w) / 2 - paintShare->arrowWsg.w - 6,
                y + (paintShare->toolbarFont.height - paintShare->arrowWsg.h) / 2, false, true, 90);
        drawWsg(&paintShare->arrowWsg, (TFT_WIDTH - w) / 2 + w + 6,
                y + (paintShare->toolbarFont.height - paintShare->arrowWsg.h) / 2, false, false, 90);
    }

    if (bottomText != NULL)
    {
        if (paintShare->canvas.h > 0 && paintShare->canvas.yScale > 0)
        {
            y = paintShare->canvas.y + paintShare->canvas.h * paintShare->canvas.yScale
                + (TFT_HEIGHT - paintShare->canvas.y - paintShare->canvas.h * paintShare->canvas.yScale
                   - paintShare->toolbarFont.height)
                      / 2;
        }
        else
        {
            y = TFT_HEIGHT - paintShare->toolbarFont.height - 8;
        }

        w = textWidth(&paintShare->toolbarFont, bottomText);
        drawText(&paintShare->toolbarFont, c000, bottomText, (TFT_WIDTH - w) / 2, y);
    }

    if (paintShare->browserVisible)
    {
        drawImageBrowser(&paintShare->browser);
    }
    else
    {
        paintBlitCanvas(&paintShare->canvas);

        // Flash the pixel we're waiting for / last sent
        if (paintShare->dataOffset != 0 && paintShare->dataOffset * 2 < paintShare->canvas.w * paintShare->canvas.h)
        {
            uint16_t x = (paintShare->dataOffset * 2) % paintShare->canvas.w;
            uint16_t y = (paintShare->dataOffset * 2) / paintShare->canvas.w;
            setPxScaled(x, y, ((paintShare->shareTime % 1000000) > 500000) ? c000 : c555, paintShare->canvas.x, paintShare->canvas.y, paintShare->canvas.xScale, paintShare->canvas.yScale);
        }
    }
}

void paintShareSendCanvas(void)
{
    PAINT_LOGI("Sending canvas metadata...");
    // Set the length to the canvas data packet length, plus one for the packet type
    paintShare->sharePacketLen = PACKET_LEN_CANVAS_DATA_V1 + 1;
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

    // Version (v1)
    paintShare->sharePacket[PAINT_MAX_COLORS + 5] = 1;

    paintShare->shareState     = SHARE_SEND_WAIT_CANVAS_DATA_ACK;
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

    paintShare->canvas.h
        = (paintShare->sharePacket[PAINT_MAX_COLORS + 1] << 8) | (paintShare->sharePacket[PAINT_MAX_COLORS + 2]);
    paintShare->canvas.w
        = (paintShare->sharePacket[PAINT_MAX_COLORS + 3] << 8) | (paintShare->sharePacket[PAINT_MAX_COLORS + 4]);

    if (paintShare->sharePacketLen > PAINT_MAX_COLORS + 5)
    {
        paintShare->version = paintShare->sharePacket[PAINT_MAX_COLORS + 5];
    }
    else
    {
        paintShare->version = 0;
    }


    uint8_t scale = paintGetMaxScale(paintShare->canvas.w, paintShare->canvas.h, SHARE_LEFT_MARGIN + SHARE_RIGHT_MARGIN,
                                     SHARE_TOP_MARGIN + SHARE_BOTTOM_MARGIN);
    paintShare->canvas.xScale = scale;
    paintShare->canvas.yScale = scale;

    paintShare->canvas.x
        = SHARE_LEFT_MARGIN
          + (TFT_WIDTH - SHARE_LEFT_MARGIN - SHARE_RIGHT_MARGIN - paintShare->canvas.w * paintShare->canvas.xScale) / 2;
    paintShare->canvas.y
        = SHARE_TOP_MARGIN
          + (TFT_HEIGHT - SHARE_TOP_MARGIN - SHARE_BOTTOM_MARGIN - paintShare->canvas.h * paintShare->canvas.yScale)
                / 2;

    paintShare->canvas.buffered = true;
    paintShare->canvas.buffer   = malloc(paintGetStoredSize(&paintShare->canvas));
    // make a sorta stripey background while we load the image
    memset(paintShare->canvas.buffer, (uint8_t)(0x10), (paintShare->canvas.w * paintShare->canvas.h + 1) / 2);
    paintShare->dataOffset = 0;

    clearPxTft();
    drawRectFilledScaled(0, 0, paintShare->canvas.w, paintShare->canvas.h, c555, paintShare->canvas.x,
                         paintShare->canvas.y, paintShare->canvas.xScale, paintShare->canvas.yScale);

    paintShare->shareState = SHARE_RECV_PIXEL_DATA;
    paintShareSendPixelRequest();
}

void paintShareSendPixels(void)
{
    // Packet type header
    paintShare->sharePacket[0] = SHARE_PACKET_PIXEL_DATA;

    // Packet seqnum
    paintShare->sharePacket[1] = (uint8_t)((paintShare->shareSeqNum >> 8) & 0xFF);
    paintShare->sharePacket[2] = (uint8_t)((paintShare->shareSeqNum >> 0) & 0xFF);

    uint8_t compatOffset = (paintShare->version == 0) ? (paintShare->shareSeqNum * 2) : 0;
    paintShare->sharePacketLen = 3 + MIN(PAINT_SHARE_PX_PACKET_LEN, (paintShare->canvas.h * paintShare->canvas.w + 1) / 2 - paintShare->dataOffset - compatOffset);

    // This will be the last packet
    if (paintShare->dataOffset + compatOffset + paintShare->sharePacketLen - 3
        >= (paintShare->canvas.w * paintShare->canvas.h))
    {
        PAINT_LOGI("This is the last packet because %d * %d + %d * 2 >= %d * %d ---> %d >= %d",
                PAINT_SHARE_PX_PER_PACKET, paintShare->shareSeqNum, PAINT_SHARE_PX_PER_PACKET, paintShare->canvas.w,
                paintShare->canvas.h, PAINT_SHARE_PX_PER_PACKET * paintShare->shareSeqNum + (paintShare->sharePacketLen - 3) * 2,
                paintShare->canvas.w * paintShare->canvas.h);
    }


    if (paintShare->canvas.buffered && paintShare->canvas.buffer)
    {
        PAINT_LOGI("Using the memcpy path with length %" PRIu8, paintShare->sharePacketLen);
        memcpy(&paintShare->sharePacket[3], &paintShare->canvas.buffer[paintShare->dataOffset + compatOffset], paintShare->sharePacketLen - 3);
    }
    else
    {
        for (uint8_t i = 0; i < paintShare->sharePacketLen - 3; i++)
        {
            // TODO dedupe this and the nvs functions into a paintSerialize() or something
            uint16_t x0 = paintShare->canvas.x
                        + ((paintShare->dataOffset * 2) + (i * 2)) % paintShare->canvas.w
                                * paintShare->canvas.xScale;
            uint16_t y0 = paintShare->canvas.y
                        + ((paintShare->dataOffset * 2) + (i * 2)) / paintShare->canvas.w
                                * paintShare->canvas.yScale;
            uint16_t x1 = paintShare->canvas.x
                        + ((paintShare->dataOffset * 2) + (i * 2 + 1)) % paintShare->canvas.w
                                * paintShare->canvas.xScale;
            uint16_t y1 = paintShare->canvas.y
                        + ((paintShare->dataOffset * 2) + (i * 2 + 1)) / paintShare->canvas.w
                                * paintShare->canvas.yScale;

            PAINT_LOGD("Mapping px(%d, %d) (%d) --> %x", x0, y0, getPxTft(x0, y0),
                    paintShare->sharePaletteMap[(uint8_t)(getPxTft(x0, y0))]);

            paintShare->sharePacket[i + 3] = paintShare->sharePaletteMap[(uint8_t)getPxTft(x0, y0)] << 4
                                            | paintShare->sharePaletteMap[(uint8_t)getPxTft(x1, y1)];
        }
    }

    paintShare->dataOffset += (paintShare->sharePacketLen - 3);

    paintShare->shareState = SHARE_SEND_WAIT_PIXEL_DATA_ACK;
    PAINT_LOGD("p2pSendMsg(%p, %d)", paintShare->sharePacket, paintShare->sharePacketLen);
    PAINT_LOGD("SENDING DATA:");
    for (uint8_t i = 0; i < paintShare->sharePacketLen; i += 4)
    {
        PAINT_LOGD("%04d %02x %02x %02x %02x", i, paintShare->sharePacket[i], paintShare->sharePacket[i + 1],
                   paintShare->sharePacket[i + 2], paintShare->sharePacket[i + 3]);
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
        return;
    }
        PAINT_LOGE("First 16 bytes: %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x %02x%02x%02x%02x",
                   paintShare->sharePacket[0], paintShare->sharePacket[1], paintShare->sharePacket[2],
                   paintShare->sharePacket[3], paintShare->sharePacket[4], paintShare->sharePacket[5],
                   paintShare->sharePacket[6], paintShare->sharePacket[7], paintShare->sharePacket[8],
                   paintShare->sharePacket[9], paintShare->sharePacket[10], paintShare->sharePacket[11],
                   paintShare->sharePacket[12], paintShare->sharePacket[13], paintShare->sharePacket[14],
                   paintShare->sharePacket[15]);
        //return;
    //}

    paintShare->shareSeqNum = (paintShare->sharePacket[1] << 8) | paintShare->sharePacket[2];

    PAINT_LOGI("Packet seqnum is %d (%x << 8 | %x)", paintShare->shareSeqNum, paintShare->sharePacket[1],
               paintShare->sharePacket[2]);

    if (paintShare->canvas.buffered)
    {
        memcpy(&paintShare->canvas.buffer[paintShare->dataOffset], &paintShare->sharePacket[3],
            paintShare->sharePacketLen - 3);
    }

    for (uint8_t i = 0; i < paintShare->sharePacketLen - 3; i++)
    {
        uint16_t x0 = ((PAINT_SHARE_PX_PER_PACKET * paintShare->shareSeqNum) + (i * 2)) % paintShare->canvas.w;
        uint16_t y0 = ((PAINT_SHARE_PX_PER_PACKET * paintShare->shareSeqNum) + (i * 2)) / paintShare->canvas.w;
        uint16_t x1 = ((PAINT_SHARE_PX_PER_PACKET * paintShare->shareSeqNum) + (i * 2 + 1)) % paintShare->canvas.w;
        uint16_t y1 = ((PAINT_SHARE_PX_PER_PACKET * paintShare->shareSeqNum) + (i * 2 + 1)) / paintShare->canvas.w;

        setPxScaled(x0, y0, paintShare->canvas.palette[paintShare->sharePacket[i + 3] >> 4], paintShare->canvas.x,
                    paintShare->canvas.y, paintShare->canvas.xScale, paintShare->canvas.yScale);
        setPxScaled(x1, y1, paintShare->canvas.palette[paintShare->sharePacket[i + 3] & 0xF], paintShare->canvas.x,
                    paintShare->canvas.y, paintShare->canvas.xScale, paintShare->canvas.yScale);
    }

    PAINT_LOGI("We've received %d / %d pixels",
               (int)paintShare->dataOffset * 2 + (paintShare->sharePacketLen - 3) * 2,
               paintShare->canvas.h * paintShare->canvas.w);

    paintShare->dataOffset += paintShare->sharePacketLen - 3;

    if (paintShare->dataOffset * 2
        >= paintShare->canvas.h * paintShare->canvas.w)
    {
        PAINT_LOGI("I think we're done receiving");
        // We don't reeeally care if the sender acks this packet.
        // I mean, it would be polite to make sure it gets there, but there's not really a point
        paintShare->shareState = SHARE_RECV_SELECT_SLOT;
        paintShareSendReceiveComplete();
    }
    else
    {
        PAINT_LOGI("Done handling pixel packet, may we please have some more?");
        paintShareSendPixelRequest();
    }
}

void paintShareSendVersion(void)
{
    paintShare->sharePacket[0] = SHARE_PACKET_VERSION;
    paintShare->sharePacket[1] = 1;
    p2pSendMsg(&paintShare->p2pInfo, paintShare->sharePacket, paintShare->sharePacketLen, paintShareP2pSendCb);
}

void paintShareSendPixelRequest(void)
{
    paintShare->sharePacket[0] = SHARE_PACKET_PIXEL_REQUEST;
    paintShare->sharePacket[1] = 1;
    paintShare->sharePacketLen = 2;
    p2pSendMsg(&paintShare->p2pInfo, paintShare->sharePacket, paintShare->sharePacketLen, paintShareP2pSendCb);
    paintShare->shareUpdateScreen = true;
}

void paintShareSendReceiveComplete(void)
{
    paintShare->sharePacket[0] = SHARE_PACKET_RECEIVE_COMPLETE;
    paintShare->sharePacketLen = 1;
    p2pSendMsg(&paintShare->p2pInfo, paintShare->sharePacket, paintShare->sharePacketLen, paintShareP2pSendCb);
    paintShare->shareUpdateScreen = true;
    paintShare->browserVisible    = false;
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

            if (PAINT_SHARE_PX_PER_PACKET * (paintShare->shareSeqNum + 1)
                >= (paintShare->canvas.w * paintShare->canvas.h))
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
            if (!paintShare->versionSent)
            {
                paintShare->versionSent = true;
            }
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

    if (paintShare->canvas.buffered && paintShare->canvas.buffer)
    {
        free(paintShare->canvas.buffer);
    }
    resetImageBrowser(&paintShare->browser);
    deinitDialogBox(paintShare->dialog);

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
            paintShare->shareState     = SHARE_SEND_CANVAS_DATA;
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
            paintShare->shareState     = SHARE_SEND_PIXEL_DATA;
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
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        paintShareButtonCb(&evt);
    }

    paintShare->shareTime += elapsedUs;
    if (paintShare->shareNewPacket)
    {
        paintShare->timeSincePacket = 0;
    }
    else if (paintShare->shareState != SHARE_SEND_SELECT_SLOT && paintShare->shareState != SHARE_RECV_SELECT_SLOT
             && paintShare->shareState != SHARE_SEND_WAIT_FOR_CONN
             && paintShare->shareState != SHARE_RECV_WAIT_FOR_CONN)
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
                    if (paintShare->sharePacketLen > 1)
                    {
                        paintShare->version = paintShare->sharePacket[1];
                    }
                    else
                    {
                        paintShare->version = 0;
                    }

                    paintShare->shareNewPacket = false;
                    paintShare->shareState     = SHARE_SEND_PIXEL_DATA;
                }
                else if (paintShare->sharePacket[0] == SHARE_PACKET_RECEIVE_COMPLETE)
                {
                    PAINT_LOGD("We've received confirmation! All data was received successfully");
                    paintShare->shareNewPacket    = false;
                    paintShare->shareState        = SHARE_SEND_COMPLETE;
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

    paintRenderShareMode(paintShare->shareTime);
}

void paintShareButtonCb(buttonEvt_t* evt)
{
    if (paintShare->shareState == SHARE_SEND_SELECT_SLOT)
    {
        switch (evt->button)
        {
            case PB_UP:
            case PB_DOWN:
            case PB_LEFT:
            case PB_RIGHT:
            case PB_SELECT:
            {
                imageBrowserButton(&paintShare->browser, evt);
                break;
            }

            case PB_A:
            {
                if (paintShare->browserVisible)
                {
                    imageBrowserButton(&paintShare->browser, evt);
                }
                else if (evt->down)
                {
                    paintBeginShare();
                }
                break;
            }

            case PB_B:
            {
                if (paintShare->browserVisible)
                {
                    imageBrowserButton(&paintShare->browser, evt);
                }
                else if (evt->down)
                {
                    // Exit without saving
                    switchToSwadgeMode(&modePaint);
                }
                break;
            }

            // Do Nothing!
            case PB_START:
                // Or do something on button up to avoid conflict with exit mode
                break;
        }
    }
    else if (paintShare->shareState == SHARE_RECV_SELECT_SLOT)
    {
        switch (evt->button)
        {
            case PB_UP:
            case PB_DOWN:
            case PB_LEFT:
            case PB_RIGHT:
            case PB_SELECT:
            {
                imageBrowserButton(&paintShare->browser, evt);
                break;
            }

            case PB_A:
            {
                if (paintShare->browserVisible)
                {
                    imageBrowserButton(&paintShare->browser, evt);
                }
                else if (!evt->down)
                {
                    paintShare->browserVisible = true;
                }
                break;
            }

            case PB_B:
            {
                if (paintShare->browserVisible)
                {
                    imageBrowserButton(&paintShare->browser, evt);
                }
                else if (evt->down)
                {
                    // Exit without saving
                    switchToSwadgeMode(&modePaint);
                }
                break;
            }

            // Do Nothing!
            case PB_START:
                // Or do something on button-up instead, to avoid overlap with SELECT+START
                break;
        }
        // Does the receiver get any buttons?
        // Yes! They need to pick their destination slot before starting P2P
    }
    else if (paintShare->shareState == SHARE_SEND_COMPLETE)
    {
        if (evt->down && evt->button == PB_B)
        {
            switchToSwadgeMode(&modePaint);
        }
        else
        {
            paintShare->shareState        = SHARE_SEND_SELECT_SLOT;
            paintShare->shareUpdateScreen = true;
            paintShare->browserVisible    = true;
        }
    }
    else if (paintShare->shareState == SHARE_SEND_WAIT_FOR_CONN || paintShare->shareState == SHARE_RECV_WAIT_FOR_CONN)
    {
        if (evt->down && evt->button == PB_B)
        {
            switchToSwadgeMode(&modePaint);
        }
    }
}

void paintShareRecvCb(const esp_now_recv_info_t* esp_now_info, const uint8_t* data, uint8_t len, int8_t rssi)
{
    p2pRecvCb(&paintShare->p2pInfo, esp_now_info->src_addr, (const uint8_t*)data, len, rssi);
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
    switch (evt)
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
                paintShare->shareState     = SHARE_SEND_CANVAS_DATA;
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
    if (len > 1 && payload[0] == SHARE_PACKET_VERSION)
    {
        // Handle the version packet immediately
        PAINT_LOGI("Got version packet: %" PRIu8, payload[1]);
        paintShare->version = payload[1];
        return;
    }

    // no buffer overruns for me thanks
    PAINT_LOGV("Receiving %d bytes via P2P callback", len);
    memcpy(paintShare->sharePacket, payload, len);

    paintShare->sharePacketLen = len;
    paintShare->shareNewPacket = true;
}

static void paintShareBrowserCb(const char* key, imageBrowserAction_t action)
{
    switch (action)
    {
        case BROWSER_EXIT:
        {
            paintShare->browserVisible = false;
            break;
        }

        case BROWSER_OPEN:
        {
            strncpy(paintShare->shareSaveSlotKey, key, sizeof(paintShare->shareSaveSlotKey) - 1);
            // Wait for confirm to share!
            paintShareDoLoad();
            paintBlitCanvas(&paintShare->canvas);
            paintShare->shareUpdateScreen = true;
            paintShare->browserVisible    = false;
            break;
        }

        case BROWSER_SAVE:
        {
            strncpy(paintShare->shareSaveSlotKey, key, sizeof(paintShare->shareSaveSlotKey) - 1);
            paintShareDoSave();
            switchToSwadgeMode(&modePaint);
            break;
        }

        case BROWSER_DELETE:
        break;
    }

    PAINT_LOGI("Share Key: %s", key);
}

void paintShareDoLoad(void)
{
    clearPxTft();

    paintShare->canvas.buffered = true;
    if (!paintLoadNamed(paintShare->shareSaveSlotKey, &paintShare->canvas))
    {
        PAINT_LOGE("Failed to load dimensions, stopping load");
        return;
    }

    // With the image dimensions, calculate the max scale that will fit on the screen
    uint8_t scale = paintGetMaxScale(paintShare->canvas.w, paintShare->canvas.h, SHARE_LEFT_MARGIN + SHARE_RIGHT_MARGIN,
                                     SHARE_TOP_MARGIN + SHARE_BOTTOM_MARGIN);
    PAINT_LOGD("Loading image at scale %d", scale);
    paintShare->canvas.xScale = scale;
    paintShare->canvas.yScale = scale;

    // Center the canvas on the empty area of the screen
    paintShare->canvas.x
        = SHARE_LEFT_MARGIN
          + (TFT_WIDTH - SHARE_LEFT_MARGIN - SHARE_RIGHT_MARGIN - paintShare->canvas.w * paintShare->canvas.xScale) / 2;
    paintShare->canvas.y
        = SHARE_TOP_MARGIN
          + (TFT_HEIGHT - SHARE_TOP_MARGIN - SHARE_BOTTOM_MARGIN - paintShare->canvas.h * paintShare->canvas.yScale)
                / 2;

    for (uint8_t i = 0; i < PAINT_MAX_COLORS; i++)
    {
        paintShare->sharePaletteMap[(uint8_t)(paintShare->canvas.palette[i])] = i;
    }
}

void paintShareDoSave(void)
{
    paintSaveNamed(paintShare->shareSaveSlotKey, &paintShare->canvas);
}
