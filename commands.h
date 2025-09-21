#ifndef COMMANDS
#define COMMANDS
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>

typedef enum _ARG_TYPES {
	FLAGS,
	ARGS
}ARG_TYPE;

typedef struct _ARG {
	ARG_TYPE type;
	char* argValue;
}ARG;

typedef struct _CONFIG {
	char* flags;        // String of flags like "wCS"
	char** values;      // Array of values for flags that need them
	int flag_count;     // Number of flags found
} CONFIG;

// Function declarations
CONFIG parseArgs(int argc, char* argv[]);
char* getConfigFile(CONFIG* config);
char* getCommandPath(CONFIG* config);
bool isSilentMode(CONFIG* config);
void freeConfig(CONFIG* config);

#endif // !COMMANDS

