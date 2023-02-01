#ifndef _SPIFFS_JSON_H_
#define _SPIFFS_JSON_H_

char* loadJson(const char* name);
void freeJson(char* jsonStr);

#endif