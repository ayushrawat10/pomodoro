#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <stdbool.h>
#include <sys/stat.h>

typedef struct {
    int workRed, workGreen, workBlue;
    int breakRed, breakGreen, breakBlue;
    int workMinutes, breakMinutes, sessions;
} Config;

char CONFIG_FILE_PATH[256] = "pomodoro_config.txt";
Config config;

void loadConfig(Config *config);
void setDefaultConfig(Config *config);
void saveConfig(Config *config, bool printMessage);
void displayTimer(Config *config);
void displayProgress(int totalSeconds, int elapsedSeconds, char* label, int session, Config *config);
void sendNotification(char* message);
void enableRawMode();
void expandUserHomeDirectory();
void handleSignal(int sig);
void disableRawMode();

void handleSignal(int sig) {
    disableRawMode();
    exit(sig);
}

void expandUserHomeDirectory() {
    char *home = getenv("HOME");
    if (home != NULL) {
        snprintf(CONFIG_FILE_PATH, sizeof(CONFIG_FILE_PATH), "%s/.config/pomodoro/pomodoro_config.txt", home);
    } 
}

void loadConfig(Config *config) {
    setDefaultConfig(config);
    expandUserHomeDirectory();

    FILE *file = fopen(CONFIG_FILE_PATH, "r");
    if (file) {
        char line[256];
        while (fgets(line, sizeof(line), file)) {
            char key[256];
            int value1, value2, value3;

            if (sscanf(line, "%[^=] = (%d,%d,%d)", key, &value1, &value2, &value3) == 4) {
                if (value1 >= 0 && value1 <= 255 && value2 >= 0 && value2 <= 255 && value3 >= 0 && value3 <= 255) {
                    if (strcmp(key, "workcolor") == 0) {
                        config->workRed = value1;
                        config->workGreen = value2;
                        config->workBlue = value3;
                    } else if (strcmp(key, "breakcolor") == 0) {
                        config->breakRed = value1;
                        config->breakGreen = value2;
                        config->breakBlue = value3;
                    }
                } else {
                    printf("Error: Invalid color values in config file. Using default values.\n");
                }
            } else if (sscanf(line, "%[^=] = %d", key, &value1) == 2) {
                if (value1 > 0) {
                    if (strcmp(key, "work") == 0) {
                        config->workMinutes = value1;
                    } else if (strcmp(key, "break") == 0) {
                        config->breakMinutes = value1;
                    } else if (strcmp(key, "sessions") == 0) {
                        config->sessions = value1;
                    }
                } else {
                    printf("Error: Invalid duration or session values in config file. Using default values.\n");
                }
            }
        }
        fclose(file);
    } else {
        setDefaultConfig(config);
    }
}

void setDefaultConfig(Config *config) {
    config->workRed = 208;
    config->workGreen = 53;
    config->workBlue = 197;

    config->breakRed = 50;
    config->breakGreen = 192;
    config->breakBlue = 50;

    config->workMinutes = 25;
    config->breakMinutes = 5;

    config->sessions = 1;
}

void saveConfig(Config *config, bool printMessage) {
    char dirPath[256];
    char *home = getenv("HOME");

    if (home != NULL) {
        snprintf(dirPath, sizeof(dirPath), "%s/.config/pomodoro", home);
    } else {
        // Home directory not found
        fprintf(stderr, "Error: home directory not found.\n");
        return;
    }

    struct stat st = {0};

    if (stat(dirPath, &st) == -1) {
        if (mkdir(dirPath, S_IRWXU) != 0) {  
            perror("Error creating directory");
            return;
        }
    }

    expandUserHomeDirectory();

    FILE *file = fopen(CONFIG_FILE_PATH, "w");

    if (!file) {
        perror("Error opening the configuration file for writing");
        return;
    }
    if (file != NULL) {
        fprintf(file, "work=%d\n", config->workMinutes);
        fprintf(file, "break=%d\n", config->breakMinutes);
        fprintf(file, "sessions=%d\n", config->sessions);
        fprintf(file, "workcolor=(%d,%d,%d)\n", config->workRed, config->workGreen, config->workBlue);
        fprintf(file, "breakcolor=(%d,%d,%d)\n", config->breakRed, config->breakGreen, config->breakBlue);
        fclose(file);

        if (printMessage) {
            printf("Pomodoro configuration saved\n");
        }
    } else {
        perror("Error saving configuration");
    }
}

void enableRawMode() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
}

void disableRawMode() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag |= (ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
}

void displayTimer(Config *config) {
    for (int s = 0; s<config->sessions; s++){
        for (int i=0; i<config->workMinutes * 60; i++) {
            displayProgress(config->workMinutes*60, i, "Work", s+1, config);
            sleep(1);
        }
        sendNotification("Work session completed!");

        if (s<config->sessions-1) {
            for (int i=0; i<config->breakMinutes * 60; i++) {
                displayProgress(config->breakMinutes * 60 , i, "Break", s+1, config);
                sleep(1);
            }
            sendNotification("Break completed! Get back to work!");
        }
    }
    printf("\nAll sessions completed! Well done!\n");
}

void displayProgress(int totalSeconds, int elapsedSeconds, char* label, int session, Config *config) {
    int remainingSeconds = totalSeconds - elapsedSeconds;
    int remainingMinutes = remainingSeconds / 60;
    remainingSeconds %= 60;

    printf("\033[H\033[J");
    printf("Session %d of %d\n", session, config->sessions);

    int remainingTime = totalSeconds / 60 - elapsedSeconds / 60;
    printf("%s for %d minute%s %d second%s.\n", 
           label, 
           remainingMinutes, remainingMinutes == 1 ? "" : "s", 
           remainingSeconds, remainingSeconds == 1 ? "" : "s");

        
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int barLength = w.ws_col - 30;
    if (barLength <0) barLength = 0;

    int completedBars = (barLength * (elapsedSeconds + 1)) / totalSeconds;
    int remainingBars = barLength - completedBars;

    printf("\r%02d:%02d %s \u2503", remainingMinutes, remainingSeconds, label);

    if (strcmp(label, "Work") == 0) {
        printf("\033[48;2;%d;%d;%dm", config->workRed, config->workGreen, config->workBlue);
    } else {
        printf("\033[48;2;%d;%d;%dm", config->breakRed, config->breakGreen, config->breakBlue);
    }
    for (int j = 0; j < completedBars; j++) printf(" ");
    printf("\033[48;2;0;0;0m");
    for (int j = 0; j < remainingBars; j++) printf(" ");
    printf("\033[0m\u2503");

    fflush(stdout);
}

void sendNotification(char* message) {
    char command[256];
    snprintf(command, sizeof(command), "osascript -e 'display notification \"%s\" with title \"Pomodoro\"'", message);
    int ret = system(command);
    if(ret == -1) {
        fprintf(stderr, "Failed to invoke system command.\n");
    }
}

void updateConfigFromCommandLine(int argc, char *argv[], Config *tempConfig, bool *runTimer) {
    if(argc > 1) {
        for (int i = 1; i < argc; ++i) {
            if (strstr(argv[i], "-")) {
                int workPeriod, breakPeriod;
                if (2 != sscanf(argv[i], "%d-%d", &workPeriod, &breakPeriod)) {
                    fprintf(stderr, "Error: Invalid work-break format. Using default values.\n");
                    continue;
                }
                tempConfig->workMinutes = (workPeriod > 0) ? workPeriod : tempConfig->workMinutes;
                tempConfig->breakMinutes = (breakPeriod > 0) ? breakPeriod : tempConfig->breakMinutes;
            } else if (strstr(argv[i], "n=")) {
                int sessions;
                if (1 != sscanf(strstr(argv[i], "=") + 1, "%d", &sessions) || sessions <= 0) {
                    fprintf(stderr, "Error: Invalid number of sessions value. Using default value.\n");
                    continue;
                }
                tempConfig->sessions = (sessions > 0) ? sessions : tempConfig->sessions;
            } else if (strcmp(argv[i], "config") == 0) {
                *runTimer = false;
                i++;
                while(i < argc) {
                    if (strstr(argv[i], "workcolor=")) {
                        int r, g, b;
                        if (3 != sscanf(strstr(argv[i], "=") + 1, "(%d,%d,%d)", &r, &g, &b) ||
                            r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
                            fprintf(stderr, "Error: Invalid workcolor format or values. Using default color.\n");
                            i++;
                            continue;
                        }
                        tempConfig->workRed = r;
                        tempConfig->workGreen = g;
                        tempConfig->workBlue = b;
                    } else if (strstr(argv[i], "breakcolor=")) {
                        int r, g, b;
                        if (3 != sscanf(strstr(argv[i], "=") + 1, "(%d,%d,%d)", &r, &g, &b) ||
                            r < 0 || r > 255 || g < 0 || g > 255 || b < 0 || b > 255) {
                            fprintf(stderr, "Error: Invalid breakcolor format or values. Using default color.\n");
                            i++;
                            continue;
                        }
                        tempConfig->breakRed = r;
                        tempConfig->breakGreen = g;
                        tempConfig->breakBlue = b;
                    } else if (strstr(argv[i], "work=")) {
                        int work;
                        if (1 != sscanf(strstr(argv[i], "=") + 1, "%d", &work) || work <= 0) {
                            fprintf(stderr, "Error: Invalid work time. Using default value.\n");
                            i++;
                            continue;
                        }
                        tempConfig->workMinutes = work;
                    } else if (strstr(argv[i], "break=")) {
                        int breakPeriod;
                        if (1 != sscanf(strstr(argv[i], "=") + 1, "%d", &breakPeriod) || breakPeriod <= 0) {
                            fprintf(stderr, "Error: Invalid break time. Using default value.\n");
                            i++;
                            continue;
                        }
                        tempConfig->breakMinutes = breakPeriod;
                    } else if (strstr(argv[i], "sessions=")) {
                        int sessions;
                        if (1 != sscanf(strstr(argv[i], "=") + 1, "%d", &sessions) || sessions <= 0) {
                            fprintf(stderr, "Error: Invalid sessions value. Using default value.\n");
                            i++;
                            continue;
                        }
                        tempConfig->sessions = sessions;
                    }
                    i++;
                }
                config = *tempConfig;
                saveConfig(&config, true);
                return;
            }
        }
    }
}

int main (int argc, char *argv[]) {
    signal(SIGINT, handleSignal);
    signal(SIGTERM, handleSignal);
    signal(SIGHUP, handleSignal);
    atexit(disableRawMode);

    enableRawMode();
    loadConfig(&config);
        
    Config tempConfig = config;
    bool runTimer = true;

    updateConfigFromCommandLine(argc, argv, &tempConfig, &runTimer);
    
    if (runTimer) {
        displayTimer(&tempConfig);
    }
    disableRawMode();
    return 0;
}
