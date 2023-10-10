#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <stdbool.h>

typedef struct {
    int workRed, workGreen, workBlue;
    int breakRed, breakGreen, breakBlue;
    int workMinutes, breakMinutes, sessions;
} Config;

const char *CONFIG_FILE_PATH = "pomodoro_config.txt";
Config config;

void loadConfig(Config *config);
void setDefaultConfig(Config *config);
void saveConfig(Config *config, bool printMessage);
void displayTimer(Config *config);
void displayProgress(int totalSeconds, int elapsedSeconds, char* label, int session, Config *config);
void sendNotification(char* message);
void enableRawMode();

void loadConfig(Config *config) {
    FILE *file = fopen(CONFIG_FILE_PATH, "r");
    if (file) {
        if (9 != fscanf(file, "%d %d %d %d %d %d %d %d %d",
                        &config->workRed, &config->workGreen, &config->workBlue, 
                        &config->breakRed, &config->breakGreen, &config->breakBlue,
                        &config->workMinutes, &config->breakMinutes, &config->sessions)) {
            setDefaultConfig(config);
            saveConfig(config, false);
        }
        fclose(file);
    } else {
        setDefaultConfig(config);
        saveConfig(config, false);
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
    FILE *file = fopen(CONFIG_FILE_PATH, "w");
    if (file != NULL) {
        fprintf(file, "%d %d %d %d %d %d %d %d %d", 
                config->workRed, config->workGreen, config->workBlue, 
                config->breakRed, config->breakGreen, config->breakBlue,
                config->workMinutes, config->breakMinutes, config->sessions);
        fclose(file);
        if (printMessage){
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
    printf("Session %d\n", session);

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
    char command[100];
    sprintf(command, "osascript -e 'display notification \"%s\" with title \"Pomodoro\"'", message);
    system(command);
}

void updateConfigFromCommandLine(int argc, char *argv[], Config *tempConfig, bool *runTimer) {
    if(argc > 1) {
        for (int i = 1; i < argc; ++i) {
            if (strstr(argv[i], "-")) {
                int workPeriod, breakPeriod;
                sscanf(argv[i], "%d-%d", &workPeriod, &breakPeriod);
                tempConfig->workMinutes = (workPeriod > 0) ? workPeriod : tempConfig->workMinutes;
                tempConfig->breakMinutes = (breakPeriod > 0) ? breakPeriod : tempConfig->breakMinutes;
            } else if (strstr(argv[i], "n=")) {
                int sessions;
                sscanf(strstr(argv[i], "=") + 1, "%d", &sessions);
                tempConfig->sessions = (sessions > 0) ? sessions : tempConfig->sessions;
            } else if (strcmp(argv[i], "config") == 0) {
                *runTimer = false;
                i++;
                while(i < argc) {
                    if (strstr(argv[i], "workcolor=")) {
                        int r, g, b;
                        sscanf(strstr(argv[i], "=") + 1, "(%d,%d,%d)", &r, &g, &b);
                        if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255) {
                            tempConfig->workRed = r;
                            tempConfig->workGreen = g;
                            tempConfig->workBlue = b;
                        }
                    } else if (strstr(argv[i], "breakcolor=")) {
                        int r, g, b;
                        sscanf(strstr(argv[i], "=") + 1, "(%d,%d,%d)", &r, &g, &b);
                        if (r >= 0 && r <= 255 && g >= 0 && g <= 255 && b >= 0 && b <= 255) {
                            tempConfig->breakRed = r;
                            tempConfig->breakGreen = g;
                            tempConfig->breakBlue = b;
                        }
                    } else if (strstr(argv[i], "work=")) {
                        int work;
                        sscanf(strstr(argv[i], "=") + 1, "%d", &work);
                        tempConfig->workMinutes = (work > 0) ? work : tempConfig->workMinutes;
                    } else if (strstr(argv[i], "break=")) {
                        int breakPeriod;
                        sscanf(strstr(argv[i], "=") + 1, "%d", &breakPeriod);
                        tempConfig->breakMinutes = (breakPeriod > 0) ? breakPeriod : tempConfig->breakMinutes;
                    } else if (strstr(argv[i], "sessions=")) {
                        int sessions;
                        sscanf(strstr(argv[i], "=") + 1, "%d", &sessions);
                        tempConfig->sessions = (sessions > 0) ? sessions : tempConfig->sessions;
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
    enableRawMode();
    loadConfig(&config);
    
    Config tempConfig = config;
    bool runTimer = true;

    updateConfigFromCommandLine(argc, argv, &tempConfig, &runTimer);
    if (runTimer) {
        displayTimer(&tempConfig);
    }
    return 0;
}
