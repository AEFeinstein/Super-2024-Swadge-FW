#include "highScores.h"

#include "fairyDraw.h"
#include "hdw-nvs.h"
#include "nameList.h"
#include "swadgePass.h"

#include <stdbool.h>
#include <stdint.h>

#define NVS_KEY_USER_HIGH_SCORE "user_high_score"
#define NVS_KEY_HIGH_SCORES     "high_scores"

#define HIGH_SCORE_COUNT(hs) \
    (hs->highScoreCount == 0 ? MAX_HIGH_SCORE_COUNT : MIN(hs->highScoreCount, MAX_HIGH_SCORE_COUNT))

void initHighScores(highScores_t* hs, const char* nvsNamespace)
{
    readNamespaceNvs32(nvsNamespace, NVS_KEY_USER_HIGH_SCORE, &hs->userHighScore);
    size_t nvsLength = sizeof(hs->highScores);
    readNamespaceNvsBlob(nvsNamespace, NVS_KEY_HIGH_SCORES, hs->highScores, &nvsLength);
}

bool updateHighScores(highScores_t* hs, const char* nvsNamespace, score_t newScores[], uint8_t numNewScores)
{
    int32_t userHighScore = hs->userHighScore;
    for (int i = 0; i < numNewScores; i++)
    {
        if (newScores[i].spKey[0] == '\0')
        {
            userHighScore = MAX(userHighScore, newScores[i].score);
        }
    }
    if (userHighScore > hs->userHighScore)
    {
        hs->userHighScore = userHighScore;
        writeNamespaceNvs32(nvsNamespace, NVS_KEY_USER_HIGH_SCORE, userHighScore);
    }

    bool changed = false;
    for (int i = 0; i < numNewScores; i++)
    {
        int pos = HIGH_SCORE_COUNT(hs);

        // Only include a single high score from any given SwadgePass user
        if (newScores[i].spKey[0] != '\0')
        {
            for (int highScorePos = 0; highScorePos < HIGH_SCORE_COUNT(hs); highScorePos++)
            {
                if (strcmp(newScores[i].spKey, hs->highScores[highScorePos].spKey) == 0)
                {
                    pos = highScorePos;
                    if (newScores[i].score > hs->highScores[highScorePos].score)
                    {
                        hs->highScores[highScorePos] = newScores[i];
                        changed                      = true;
                    }
                }
            }
        }

        // Insert new score at the correct position in high score array
        while (pos > 0 && newScores[i].score > hs->highScores[pos - 1].score)
        {
            if (pos < HIGH_SCORE_COUNT(hs))
            {
                hs->highScores[pos] = hs->highScores[pos - 1];
            }
            changed = true;
            pos--;
        }
        if (pos < HIGH_SCORE_COUNT(hs) && newScores[i].score > hs->highScores[pos].score)
        {
            hs->highScores[pos] = newScores[i];
            changed             = true;
        }
    }

    // Ensure we didn't kick all the user's scores off the leaderboard
    bool userScoreExists = false;
    for (int i = 0; i < HIGH_SCORE_COUNT(hs); i++)
    {
        if (hs->highScores[i].spKey[0] == '\0')
        {
            userScoreExists = true;
            break;
        }
    }
    if (!userScoreExists)
    {
        memset(&hs->highScores[HIGH_SCORE_COUNT(hs) - 1], 0, sizeof(score_t));
        hs->highScores[HIGH_SCORE_COUNT(hs) - 1].score = hs->userHighScore;
        changed                                        = true;
    }

    if (changed)
    {
        writeNamespaceNvsBlob(nvsNamespace, NVS_KEY_HIGH_SCORES, hs->highScores, hs->highScoreCount * sizeof(score_t));
    }

    return changed;
}

void saveHighScoresFromSwadgePass(highScores_t* hs, const char* nvsNamespace, list_t swadgePasses,
                                  const struct swadgeMode* mode,
                                  int32_t (*fnGetSwadgePassHighScore)(const swadgePassPacket_t* packet))
{
    if (swadgePasses.length > 0)
    {
        score_t spScores[swadgePasses.length];
        int i        = 0;
        node_t* node = swadgePasses.first;
        while (node)
        {
            swadgePassData_t* spd = node->val;
            spScores[i].score     = fnGetSwadgePassHighScore(&spd->data.packet);
            memcpy(&spScores[i].spKey, &spd->key, NVS_KEY_NAME_MAX_SIZE);
            spScores[i].packedName = spd->data.packet.username;
            memcpy(&spScores[i].fairy, &spd->data.packet.fairyCol.fairy, sizeof(fairy_t));
            i++;
            node = node->next;
        }
        updateHighScores(hs, nvsNamespace, spScores, ARRAY_SIZE(spScores));

        // Switching to speaker disables the microphone
        switchToSpeaker();
        // This disables speaker too
        dacStop();

        node = swadgePasses.first;
        while (node)
        {
            setPacketUsedByMode(node->val, mode, true);
            node = node->next;
        }

        // This re-enables the speakers
        dacStart();
    }
}

void clearHighScores(const char* nvsNamespace)
{
    eraseNamespaceNvsKey(nvsNamespace, NVS_KEY_USER_HIGH_SCORE);
    eraseNamespaceNvsKey(nvsNamespace, NVS_KEY_HIGH_SCORES);
}

void addHighScoreToSwadgePassPacket(const char* nvsNamespace, swadgePassPacket_t* packet,
                                    void (*fnSetSwadgePassHighScore)(swadgePassPacket_t* packet, int32_t highScore))
{
    int32_t highScore = 0;
    readNamespaceNvs32(nvsNamespace, NVS_KEY_USER_HIGH_SCORE, &highScore);
    fnSetSwadgePassHighScore(packet, highScore);
}

void initDisplayHighScores(highScores_t* hs, displayScore_t ds[])
{
    fairy_t fairy;
    for (int i = 0; i < hs->highScoreCount; i++)
    {
        if (hs->highScores[i].score > 0)
        {
            if (ds[i].image.w != 0)
            {
                freeWsg(&ds[i].image);
            }

            if (hs->highScores[i].spKey[0] == '\0')
            {
                nameData_t username = *getSystemUsername();
                memcpy(&ds[i].name, &username, sizeof(nameData_t));

                size_t size = sizeof(fairy_t);
                readNamespaceNvsBlob(nvsStrs[FC_NAMESPACE], nvsStrs[FC_USER_FAIRY], &fairy, &size);
            }
            else
            {
                setUsernameFrom32(&ds[i].name, hs->highScores[i].packedName);

                memcpy(&fairy, &hs->highScores[i].fairy, sizeof(fairy_t));
            }

            fcGenerateFairyImage(&fairy, &ds[i].image, false);
        }
    }
}

void freeDisplayHighScores(highScores_t* hs, displayScore_t ds[])
{
    for (int i = 0; i < hs->highScoreCount; i++)
    {
        if (ds[i].image.w != 0)
        {
            freeWsg(&ds[i].image);
        }
    }
}
