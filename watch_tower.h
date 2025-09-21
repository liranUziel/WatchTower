#ifndef WATCH_TOWER_H
#define WATCH_TOWER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <windows.h>
#include <wincrypt.h>
typedef enum TYPE {
	CMD
}TYPE;

typedef struct WATCHER{
    int id;
    TYPE type;
    char** paths;
    int path_count;
    char* script;
}WATCHER;

int readWATCHfile(const char* json_path);
int parserWATCHfile(FILE* file, WATCHER* watcher);
void cleanUp(WATCHER* watcher);
void printWATCHER(WATCHER watcher);

int readWatchFileContent(const char* path, char** data);
char* hash_file(const char* data, DWORD data_len);
void hashingTable(WATCHER watcher);
DWORD WINAPI WatcherThread(LPVOID param);
HANDLE runCMDfromCurrent(const char* script);
HANDLE runPYTHONfromCurrent(const char* scriptPath);
void startWatching(const WATCHER watcher);

#endif