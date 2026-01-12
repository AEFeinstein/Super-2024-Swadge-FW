/*
 * nvs_manager.h
 *
 *  Created on: 3 Dec 2022
 *      Author: bryce and dylwhich
 */

#ifndef _NVS_MANAGER_H
#define _NVS_MANAGER_H

#include "swadge2024.h"

extern swadgeMode_t modeNvsManager;


/*==============================================================================
 * Prototypes
 *============================================================================*/

void nvsManagerEnterMode(void);
void nvsManagerExitMode(void);
void nvsManagerButtonCallback(buttonEvt_t* evt);
void nvsManagerMainLoop(int64_t elapsedUs);
void nvsManagerBackgroundDrawCb(int16_t x, int16_t y, int16_t w, int16_t h, int16_t up, int16_t upNum);
void nvsManagerSetUpTopMenu(bool resetPos);
void nvsManagerTopLevelCb(const char* opt);
void nvsManagerSetUpManageDataMenu(bool resetPos);
void nvsManagerManageDataCb(const char* opt);
bool nvsManagerReadAllNvsEntryInfos();
char* blobToStrWithPrefix(const void * value, size_t length);
const char* getNvsTypeName(nvs_type_t type);


#endif /* _NVS_MANAGER_H */
