#pragma once

//==============================================================================
// Includes
//==============================================================================

#include "ggCommonTypes.h"

//==============================================================================
// Function definitions
//==============================================================================

/**
 * @brief Calculates the scores for the end of each level
 * 
 * @param ggd Game Data
 * @param urinalScores Final scores for the urinal
 * @param best Best urinal option score
 * @param worst Worst urinal option score
 */
void ggCalcFinalScores(ggData_t* ggd, int* urinalScores, int* best, int* worst);

/**
 * @brief Calculates the score of each Urinal
 * 
 * @param urinals List of urinals to evaluate 
 * @param numActive Number of active urinals
 * @param urinalScores Array of scores, one for each urinal in order from left to right
 * @param best Best possible score
 * @param worst Worst possible score
 */
void ggCalcUrinalScores(ggUrinal_t* urinals, int numActive, int* urinalScores, int* best, int* worst);

// NVS
/**
 * @brief Saves the current score to NVS. Automatically called by ggCalcFinalScores()
 * 
 * @param ggd Game Data
 */
void ggSaveFinalToNVS(ggData_t* ggd);