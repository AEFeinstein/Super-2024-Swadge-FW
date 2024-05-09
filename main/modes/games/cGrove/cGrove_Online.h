#ifndef _CGROVE_ONLINE_H_
#define _CGROVE_ONLINE_H_

#include "swadge2024.h"
#include "cGrove_Types.h"
#include "cGrove_Helpers.h"

void cGroveProfileMain(cGrove_t*);
void cGroveShowMainProfile(playerProfile_t, font_t);
void cGroveShowSubProfile(cGrove_t*);
void cGroveToggleOnlineFeatures(cGrove_t*);
void cGroveEspNowRecvCb(const esp_now_recv_info_t*, const uint8_t*, uint8_t, int8_t);
void cGroveEspNowSendCb(const uint8_t*, esp_now_send_status_t);

#endif