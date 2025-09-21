#include "watch_tower.h"
#define MAX_LINE 512
#define MAX_PATHS 100

int readWATCHfile(const char* json_path) {
	FILE* file = fopen(json_path, "rb");
	int i;
	if (!file) {
		fprintf(stderr, "Failed to open file: %s\n", json_path);
		return 1;
	}
    WATCHER watcher = { 0 };
    watcher.paths = malloc(sizeof(char*) * MAX_PATHS);
    if (!watcher.paths) {
        fprintf(stderr, "Failed to allocate memory for paths\n");
        fclose(file);
        return 1;
    }
    watcher.path_count = 0;
    parserWATCHfile(file, &watcher);
    startWatching(watcher);
	return 0;
}
void startWatching(const WATCHER watcher) {

    char* hashes[MAX_PATHS];
    HANDLE thread = CreateThread(NULL, 0, WatcherThread, &watcher, 0, NULL);
    if (!thread) {
        fprintf(stderr, "Failed to create watcher thread\n");
        cleanUp(&watcher);
        return;
    }
    WaitForSingleObject(thread, INFINITE);
    CloseHandle(thread);
    cleanUp(&watcher);
}


int parserWATCHfile(FILE* file, WATCHER *watcher) {
    
    char line[MAX_LINE];
    int line_number = 0;
    int state = 0;
    while (fgets(line, sizeof(line), file)) 
    { 
        line_number++;
        line[strcspn(line, "\r\n")] = 0;
        if (strlen(line) == 0)
        {
            continue;
        } 
        if (strcmp(line, "type:WATCHER") == 0 && state == 0) {
            // File indecator
            //fprintf(stdout, "Found matching type\n");
            state = 1;
        }
        if (strcmp(line, "paths:") == 0 && state == 1) {
            //fprintf(stdout, "Start reading paths...\n");
            state = 2;
        }
        if (state == 2 && line[0] == '[') {
            // Start of paths
            state = 3;
        }
        if (state == 3 && line[0] == '"') {
            // Add paths
            char* start = strchr(line, '"');
            char* end = strrchr(line, '"');
            if (start && end && end > start) {
                int len = end - start - 1;
                watcher->paths[watcher->path_count] = malloc(sizeof(char) * (len + 1));
                memcpy(watcher->paths[watcher->path_count], start + 1, len);
                watcher->paths[watcher->path_count][len] = '\0';
                watcher->path_count++;
            }
        }
        else if (state == 3 && line[0] == ']') {
            // No more paths
            state = 4;
        }
        if (state == 4 && strcmp(line, "script:CMD") == 0) {
            state = 5;
        }
        if (state == 5 && line[0] == '"') {
            char* start = strchr(line, '"');
            char* end = strrchr(line, '"');
            if (start && end && end > start) {
                int len = end - start - 1;
                watcher->script = malloc(sizeof(char) * (len + 1));
                if (watcher->script) {
                    memcpy(watcher->script, start + 1, len);
                    watcher->script[len] = '\0';
                    watcher->type = CMD;
                }
                else {
                    fprintf(stderr, "Failed to allocate memory for script.\n");
                    return 1;
                }
            }
        }
    }

    fclose(file);
    return 0;
}

void printWATCHER(WATCHER watcher) {
    
    int i;
    fprintf(stdout, "Type: %s\n", watcher.type == CMD ? "CMD" : "UNKNOWN");
    fprintf(stdout, "PATHs:\n");
    for (i = 0;i < watcher.path_count;i++) {
        fprintf(stdout, "[%d]:[%s]\n", i, watcher.paths[i]);
    }
}

void cleanUp(WATCHER *watcher) {
    for (int i = 0; i < watcher->path_count; i++) {
        free(watcher->paths[i]);
    }
    free(watcher->paths);
    free(watcher->script);

    watcher->paths = NULL;
    watcher->script = NULL;
    watcher->path_count = 0;
}

void hashingTable(WATCHER watcher) {
    char total_hash[65] = { 0 }; // Final combined hash
    char temp_hash[65];
    int i;

    // Start with an empty string
    strcpy_s(total_hash, sizeof(total_hash), "");

    for (i = 0; i < watcher.path_count; i++) {
        char* content = NULL;
        if (readWatchFileContent(watcher.paths[i], &content) != 0) {
            fprintf(stderr, "Failed to read file: %s\n", watcher.paths[i]);
            continue;
        }

        char* hash = hash_file(content, strlen(content));
        if (!hash) {
            fprintf(stderr, "Failed to hash file: %s\n", watcher.paths[i]);
            free(content);
            continue;
        }

        // Mix into total hash by XORing characters with index offset
        for (int j = 0; j < 64; j++) {
            temp_hash[j] = total_hash[j] ^ (hash[j] + i);
        }
        temp_hash[64] = '\0';
        strcpy_s(total_hash, sizeof(total_hash), temp_hash);

        free(content);
        free(hash);
    }
}

int readWatchFileContent(const char* path, char** data) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Failed to open file: %s\n", path);
        return 1;
    }
    long length;
    fseek(file, 0, SEEK_END);
    length = ftell(file);
    rewind(file);

    *data = (char*)malloc(length + 1);
    if (!*data) {
        fclose(file);
        return 1;
    }

    fread(*data, 1, length, file);
    (*data)[length] = '\0';
    fclose(file);
    return 0;
}

char* hash_file(const char* data, DWORD data_len) {
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BYTE hash[32]; // SHA-256 is 32 bytes
    DWORD hash_len = 32;
    char* hex = malloc(65); // 64 chars + null terminator

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        return NULL;
    }

    if (!CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
        CryptReleaseContext(hProv, 0);
        return NULL;
    }

    if (!CryptHashData(hHash, (BYTE*)data, data_len, 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return NULL;
    }

    if (!CryptGetHashParam(hHash, HP_HASHVAL, hash, &hash_len, 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return NULL;
    }

    for (int i = 0; i < hash_len; i++) {
        sprintf_s(hex + (i * 2), 3, "%02x", hash[i]);
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return hex;
}
DWORD WINAPI WatcherThread(LPVOID param) {
    WATCHER* watcher = (WATCHER*)param;
    char* hashes[MAX_PATHS] = { 0 };
    HANDLE scriptHandle = NULL;
    for (int i = 0; i < watcher->path_count; i++) {
        char* content = NULL;
        if (readWatchFileContent(watcher->paths[i], &content) == 0) {
            hashes[i] = hash_file(content, strlen(content));
            free(content);
        }
    }
    // Start process at beginning
    if (watcher->type == CMD) {
        scriptHandle = runCMDfromCurrent(watcher->script);
    }
    fprintf(stdout, "Watching...\n");
    while (1) {
        Sleep(1000);
        for (int i = 0; i < watcher->path_count; i++) {
            char* content = NULL;
            if (readWatchFileContent(watcher->paths[i], &content) == 0) {
                char* new_hash = hash_file(content, strlen(content));
                if (new_hash && strcmp(new_hash, hashes[i]) != 0) {
                    // If process is running, terminate and restart
                    if (scriptHandle != NULL) {
                        TerminateProcess(scriptHandle, 0);
                        CloseHandle(scriptHandle);
                        scriptHandle = NULL;
                    }
                    if (watcher->type == CMD) {
                        scriptHandle = runCMDfromCurrent(watcher->script);
                    }
                    free(hashes[i]);
                    hashes[i] = new_hash;
                }
                else {
                    free(new_hash);
                }
                free(content);
            }
        }
        // Check if process is still running
        if (scriptHandle != NULL) {
            DWORD exitCode = 0;
            if (!GetExitCodeProcess(scriptHandle, &exitCode) || exitCode != STILL_ACTIVE) {
                CloseHandle(scriptHandle);
                scriptHandle = NULL;
            }
        }
    }
    for (int i = 0; i < watcher->path_count; i++) {
        free(hashes[i]);
    }

    return 0;
}


HANDLE runCMDfromCurrent(const char* script) {

    // Check if script starts with "python "
    if (strncmp(script, "python ", 7) == 0) {
        // Delegate to runPYTHONfromCurrent
        return runPYTHONfromCurrent(script + 7);
    }

    char exePath[MAX_PATH];
    DWORD len = GetCurrentDirectoryA(MAX_PATH, exePath);
    exePath[len] = '\0';
    if (len == 0 || len == MAX_PATH) {
        fprintf(stderr, "Failed to get executable path\n");
        return NULL;
    }

    // Build full path to script (demo.exe) only
    char fullScriptPath[MAX_PATH * 2];
    snprintf(fullScriptPath, sizeof(fullScriptPath), "%s\\%s", exePath, script);

    // Build command to run with cmd.exe
    char command[MAX_PATH * 2 + 32];
    snprintf(command, sizeof(command), "cmd.exe /C \"%s\"", fullScriptPath);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    BOOL success = CreateProcessA(
        NULL,
        command,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    );

    if (!success) {
        fprintf(stderr, "Failed to run: %s\n", command);
        return NULL;
    }

    // Return process handle
    return pi.hProcess;
}

HANDLE runPYTHONfromCurrent(const char* scriptPath) {
    char exePath[MAX_PATH];
    DWORD len = GetCurrentDirectoryA(MAX_PATH, exePath);
    exePath[len] = '\0';
    if (len == 0 || len == MAX_PATH) {
        fprintf(stderr, "Failed to get executable path\n");
        return NULL;
    }

    // Build full path to python script
    char fullScriptPath[MAX_PATH * 2];
    snprintf(fullScriptPath, sizeof(fullScriptPath), "%s\\%s", exePath, scriptPath);

    // Build command to run with cmd.exe
    char command[MAX_PATH * 2 + 32];
    snprintf(command, sizeof(command), "cmd.exe /C \"python \"%s\"\"", fullScriptPath);

    STARTUPINFOA si;
    PROCESS_INFORMATION pi;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    BOOL success = CreateProcessA(
        NULL,
        command,
        NULL,
        NULL,
        FALSE,
        0,
        NULL,
        NULL,
        &si,
        &pi
    );

    if (!success) {
        fprintf(stderr, "Failed to run: %s\n", command);
        return NULL;
    }

    // Return process handle
    return pi.hProcess;
}