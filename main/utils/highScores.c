#include "highScores.h"

#include "esp_log.h"
#include "macros.h"
#include "../../components/hdw-nvs/include/hdw-nvs.h"

#include <stdbool.h>
#include <stdint.h>

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
        if (newScores[i].swadgePassUsername == 0)
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
        if (newScores[i].swadgePassUsername != 0)
        {
            for (int j = 0; j < HIGH_SCORE_COUNT(hs); j++)
            {
                if (newScores[i].swadgePassUsername == hs->highScores[j].swadgePassUsername)
                {
                    pos = j;
                    if (newScores[i].score > hs->highScores[j].score)
                    {
                        hs->highScores[j] = newScores[i];
                        changed           = true;
                    }
                }
            }
        }

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
        if (hs->highScores[i].swadgePassUsername == 0)
        {
            userScoreExists = true;
            break;
        }
    }
    if (!userScoreExists)
    {
        hs->highScores[HIGH_SCORE_COUNT(hs) - 1].score              = hs->userHighScore;
        hs->highScores[HIGH_SCORE_COUNT(hs) - 1].swadgePassUsername = 0;
        changed                                                     = true;
    }

    if (changed)
    {
        writeNamespaceNvsBlob(nvsNamespace, NVS_KEY_HIGH_SCORES, hs->highScores, sizeof(hs->highScores));
    }

    return changed;
}
