#include "highScoreTest.h"

#include "highScores.h"
#include "mainMenu.h"

static const char highScoreTestName[] = "High Score Test";
static const char hstViewTableLbl[]   = "View High Score Table";
static const char hstAddScoreLbl[]    = "Add Score (High: %" PRIi32 ")";
static const char hstAddSpScoreLbl[]  = "Add SwadgePass Score";
static const char hstClearScoresLbl[] = "Clear Scores";
static const char hstExitLbl[]        = "Exit";

static const char HST_NVS_NAMESPACE[] = "highScoreTest";

#define HIGH_SCORE_COUNT 9

typedef enum
{
    /// Main menu
    HST_MENU,
    /// High scores screen
    HST_HIGH_SCORES,
} hstState;

typedef struct
{
    hstState state;

    menuZorldoRenderer_t* menuRenderer;
    menu_t* menu;
    const char addScoreLabel[64];

    highScores_t highScores;
    displayScore_t displayScores[HIGH_SCORE_COUNT];
} highScoreTest_t;
highScoreTest_t* hst = NULL;

static void hstEnterMode(void);
static void hstExitMode(void);
static bool hstMenu(const char* label, bool selected, uint32_t value);
static void hstSetUserHighScoreLabel();
static void hstMainLoop(int64_t elapsedUs);
static void hstBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);

#ifdef CONFIG_BUILD_TYPE_DEBUG
static void hstAddToSwadgePassPacket(swadgePassPacket_t* packet);
static int32_t hstGetSwadgePassHighScore(const swadgePassPacket_t* packet);
static void hstSetSwadgePassHighScore(swadgePassPacket_t* packet, int32_t highScore);
#endif

swadgeMode_t highScoreTestMode = {
    .modeName                 = highScoreTestName,
    .wifiMode                 = NO_WIFI,
    .fnEnterMode              = hstEnterMode,
    .fnExitMode               = hstExitMode,
    .fnMainLoop               = hstMainLoop,
    .fnBackgroundDrawCallback = hstBackgroundDrawCallback,
#ifdef CONFIG_BUILD_TYPE_DEBUG
    .fnAddToSwadgePassPacket = hstAddToSwadgePassPacket,
#endif
};

static void hstEnterMode(void)
{
    hst = heap_caps_calloc(1, sizeof(highScoreTest_t), MALLOC_CAP_8BIT);

    hst->menu = initMenu(highScoreTestName, hstMenu);
    addSingleItemToMenu(hst->menu, hstViewTableLbl);
    settingParam_t scoreBounds = {
        .min = 1,
        .max = 9999,
    };
    addSettingsItemToMenu(hst->menu, hst->addScoreLabel, &scoreBounds, 1);
    addSettingsItemToMenu(hst->menu, hstAddSpScoreLbl, &scoreBounds, 1);
    addSingleItemToMenu(hst->menu, hstClearScoresLbl);
    addSingleItemToMenu(hst->menu, hstExitLbl);
    hst->menuRenderer = initMenuZorldoRenderer(NULL, NULL);

    hst->highScores.highScoreCount = HIGH_SCORE_COUNT;
    initHighScores(&hst->highScores, HST_NVS_NAMESPACE);
    hstSetUserHighScoreLabel();

#ifdef CONFIG_BUILD_TYPE_DEBUG
    list_t swadgePasses = {0};
    getSwadgePasses(&swadgePasses, &highScoreTestMode, false);
    saveHighScoresFromSwadgePass(&hst->highScores, HST_NVS_NAMESPACE, swadgePasses, &highScoreTestMode,
                                 hstGetSwadgePassHighScore);
    freeSwadgePasses(&swadgePasses);
#endif
}

static void hstExitMode(void)
{
    deinitMenuZorldoRenderer(hst->menuRenderer);
    deinitMenu(hst->menu);

    freeDisplayHighScores(&hst->highScores, hst->displayScores);

    heap_caps_free(hst);
}

static bool hstMenu(const char* label, bool selected, uint32_t value)
{
    if (selected)
    {
        if (label == hstViewTableLbl)
        {
            initDisplayHighScores(&hst->highScores, hst->displayScores);
            hst->state = HST_HIGH_SCORES;
        }
        else if (label == hst->addScoreLabel)
        {
            score_t scores[] = {{.score = value, .spKey = {0}, .packedName = 0, .fairy = {0}}};
            updateHighScores(&hst->highScores, HST_NVS_NAMESPACE, scores, ARRAY_SIZE(scores));
            hstSetUserHighScoreLabel();
        }
        else if (label == hstAddSpScoreLbl)
        {
            score_t scores[] = {{.score = value}};

            for (int i = 0; i < ARRAY_SIZE(scores[0].spKey); i++)
            {
                scores[0].spKey[i] = esp_random() % 10 + 48;
            }

            nameData_t nameData;
            generateRandUsername(&nameData);
            scores[0].packedName = GET_PACKED_USERNAME(nameData);

            fairy_t fairy = {
                .shape    = esp_random() % FC_SHAPE_COUNT,
                .charm    = esp_random() % FC_CHARM_COUNT,
                .filling  = esp_random() % FC_FILL_COUNT,
                .pedestal = esp_random() % FC_PED_COUNT,
                .color    = esp_random() % FC_COL_COUNT,
                .wing     = esp_random() % FC_WING_COUNT,
                .ball     = esp_random() % FC_BALL_COUNT,
                .aura     = esp_random() % FC_AURA_COUNT,
            };
            memcpy(&scores[0].fairy, &fairy, sizeof(fairy_t));

            updateHighScores(&hst->highScores, HST_NVS_NAMESPACE, scores, ARRAY_SIZE(scores));
        }
        else if (label == hstClearScoresLbl)
        {
            clearHighScores(HST_NVS_NAMESPACE);
            hst->highScores.userHighScore = 0;
            memset(hst->highScores.highScores, 0, sizeof(score_t) * MAX_HIGH_SCORE_COUNT);
            hstSetUserHighScoreLabel();
        }
        else if (label == hstExitLbl)
        {
            switchToSwadgeMode(&mainMenuMode);
        }
    }
    return false;
}

static void hstSetUserHighScoreLabel()
{
    sprintf((char*)hst->addScoreLabel, hstAddScoreLbl, hst->highScores.userHighScore);
}

static void hstMainLoop(int64_t elapsedUs)
{
    buttonEvt_t evt = {0};
    while (checkButtonQueueWrapper(&evt))
    {
        if (hst->state == HST_MENU)
        {
            hst->menu = menuButton(hst->menu, evt);
        }
        else if (hst->state == HST_HIGH_SCORES)
        {
            if ((evt.button == PB_A || evt.button == PB_B) && evt.down)
            {
                freeDisplayHighScores(&hst->highScores, hst->displayScores);
                hst->state = HST_MENU;
            }
        }
    }

    switch (hst->state)
    {
        case HST_MENU:
        {
            drawMenuZorldo(hst->menu, hst->menuRenderer, elapsedUs);
            break;
        }

        case HST_HIGH_SCORES:
        {
            int16_t yOff        = 15;
            uint16_t scoreWidth = textWidth(getSysFont(), "0000");
            for (int i = 0; i < HIGH_SCORE_COUNT; i++)
            {
                if (hst->highScores.highScores[i].score > 0)
                {
                    drawWsgSimpleHalf(&hst->displayScores[i].image, 12, yOff - 7);
                    drawTextEllipsize(getSysFont(), c000, hst->displayScores[i].name.nameBuffer, 45, yOff,
                                      TFT_WIDTH - 67 - scoreWidth - 5, false);

                    char buf[16];
                    snprintf(buf, sizeof(buf), "%" PRIi32, hst->highScores.highScores[i].score);
                    int16_t tw = textWidth(getSysFont(), buf);
                    drawText(getSysFont(), c000, buf, TFT_WIDTH - tw - 18, yOff);
                    yOff += getSysFont()->height + 15;
                }
            }

            break;
        }
    }
}

static void hstBackgroundDrawCallback(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum)
{
    fillDisplayArea(x, y, x + w, y + h, c344);
}

#ifdef CONFIG_BUILD_TYPE_DEBUG
static void hstAddToSwadgePassPacket(swadgePassPacket_t* packet)
{
    addHighScoreToSwadgePassPacket(HST_NVS_NAMESPACE, packet, hstSetSwadgePassHighScore);
}

static int32_t hstGetSwadgePassHighScore(const swadgePassPacket_t* packet)
{
    return packet->highScoreTest.highScore;
}

static void hstSetSwadgePassHighScore(swadgePassPacket_t* packet, int32_t highScore)
{
    packet->highScoreTest.highScore = highScore;
}
#endif
