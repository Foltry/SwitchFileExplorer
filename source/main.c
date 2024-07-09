#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <switch.h>

#define PATH_MAX 1024

char currentPath[PATH_MAX] = "/";
char entries[256][PATH_MAX];
int entryCount = 0;
int selected = 0;

void listDirectory(const char* path) {
    DIR* dir = opendir(path);
    if (!dir) {
        printf("Error: Unable to open directory %s\n", path);
        return;
    }

    struct dirent* entry;
    entryCount = 0;

    // Add the "..." entry for going back if not in root directory
    if (strcmp(path, "/") != 0) {
        snprintf(entries[entryCount++], PATH_MAX, "...");
    }

    while ((entry = readdir(dir)) != NULL && entryCount < 256) {
        snprintf(entries[entryCount++], PATH_MAX, "%s", entry->d_name);
    }
    closedir(dir);
}

void goBack(char* path) {
    char* lastSlash = strrchr(path, '/');
    if (lastSlash != NULL && lastSlash != path) {
        *lastSlash = '\0';
    } else {
        strcpy(path, "/");
    }
}

void deleteFile(const char* path) {
    struct stat pathStat;
    if (stat(path, &pathStat) == 0) {
        if (S_ISDIR(pathStat.st_mode)) {
            if (rmdir(path) != 0) {
                printf("Error: Unable to delete directory %s\n", path);
            } else {
                printf("Directory %s deleted.\n", path);
            }
        } else {
            if (unlink(path) != 0) {
                printf("Error: Unable to delete file %s\n", path);
            } else {
                printf("File %s deleted.\n", path);
            }
        }
    }
}

void createDirectory(const char* path) {
    if (mkdir(path, 0777) != 0) {
        printf("Error: Unable to create directory %s\n", path);
    } else {
        printf("Directory %s created.\n", path);
    }
}

void display() {
    consoleClear();
    printf("Current path: %s\n", currentPath);
    for (int i = 0; i < entryCount; i++) {
        if (i == selected) {
            printf("> %s\n", entries[i]);
        } else {
            printf("  %s\n", entries[i]);
        }
    }
    printf("\nControls:\n");
    printf("  Up/Down: Navigate\n");
    printf("  A: Enter directory\n");
    printf("  B: [Disabled]\n");
    printf("  X: Delete file/directory\n");
    printf("  Y: Create new folder\n");
    printf("  +: Exit\n");

    consoleUpdate(NULL);
}

void getUserInput(char* buffer, int bufferSize) {
    SwkbdConfig kbd;
    swkbdCreate(&kbd, 0);
    swkbdConfigMakePresetDefault(&kbd);
    swkbdConfigSetGuideText(&kbd, "Enter folder name");
    swkbdShow(&kbd, buffer, bufferSize);
    swkbdClose(&kbd);
}

int main(int argc, char* argv[]) {
    consoleInit(NULL);
    padConfigureInput(1, HidNpadStyleSet_NpadStandard);
    PadState pad;
    padInitializeDefault(&pad);

    printf("File Explorer\n");

    listDirectory(currentPath);
    display();

    while (appletMainLoop()) {
        padUpdate(&pad);
        u64 kDown = padGetButtonsDown(&pad);

        if (kDown & HidNpadButton_Plus) break;
        
        // Disable button B for now
        // if (kDown & HidNpadButton_B) {
        //     goBack(currentPath);
        //     listDirectory(currentPath);
        //     selected = 0;
        //     display();
        // }
        
        if (kDown & HidNpadButton_Up) {
            if (selected > 0) selected--;
            display();
        }
        if (kDown & HidNpadButton_Down) {
            if (selected < entryCount - 1) selected++;
            display();
        }
        if (kDown & HidNpadButton_A) {
            if (strcmp(entries[selected], "...") == 0) {
                goBack(currentPath);
            } else {
                char newPath[PATH_MAX];
                if (snprintf(newPath, PATH_MAX, "%s/%s", currentPath, entries[selected]) >= PATH_MAX) {
                    printf("Error: Path too long\n");
                    continue;
                }
                struct stat pathStat;
                if (stat(newPath, &pathStat) == 0) {
                    if (S_ISDIR(pathStat.st_mode)) {
                        strncpy(currentPath, newPath, PATH_MAX - 1);
                        currentPath[PATH_MAX - 1] = '\0';
                    } else {
                        printf("%s is not a directory.\n", newPath);
                    }
                } else {
                    printf("Error: Unable to stat path %s\n", newPath);
                }
            }
            listDirectory(currentPath);
            selected = 0;
            display();
        }
        if (kDown & HidNpadButton_X) {
            char delPath[PATH_MAX];
            snprintf(delPath, PATH_MAX, "%s/%s", currentPath, entries[selected]);
            deleteFile(delPath);
            listDirectory(currentPath);
            display();
        }
        if (kDown & HidNpadButton_Y) {
            char folderName[256] = {0};
            getUserInput(folderName, sizeof(folderName));
            if (strlen(folderName) > 0) {
                char newDirPath[PATH_MAX];
                snprintf(newDirPath, PATH_MAX, "%s/%s", currentPath, folderName);
                createDirectory(newDirPath);
                listDirectory(currentPath);
                display();
            }
        }

        consoleUpdate(NULL);
    }

    consoleExit(NULL);
    return 0;
}
