#pragma once

typedef int (*consoleCommandCb_t)(const char** args, int argCount, char* out);

typedef struct
{
    const char* name;
    consoleCommandCb_t cb;
} consoleCommand_t;

const consoleCommand_t* getConsoleCommands(void);
int consoleCommandCount(void);
