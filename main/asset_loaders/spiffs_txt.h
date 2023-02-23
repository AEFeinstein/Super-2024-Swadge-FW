#ifndef _SPIFFS_TXT_H_
#define _SPIFFS_TXT_H_

char* loadTxt(const char* name, bool spiRam);
void freeTxt(char* txtStr);

#endif