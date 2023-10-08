#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <stdbool.h>

void enableRawMode();
void displayTimer(int workMinutes, int breakMinutes, int sessions);
void displayProgress(int totalSeconds, int elapsedSeconds, char* label, int session);
void sendNotification(char* message);

void enableRawMode() {
    struct termios term;
    tcgetattr(STDIN_FILENO, &term);
    term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &term);
}

void displayTimer(int workMinutes, int breakMinutes, int sessions) {
    for (int s = 0; s<sessions; s++){
        for (int i=0; i<workMinutes * 60; i++) {
            displayProgress(workMinutes*60, i, "Work", s+1);
            sleep(1);
        }
        sendNotification("Work session completed!");


        if (sessions>1) {
            for (int i=0; i<breakMinutes * 60; i++) {
                displayProgress(breakMinutes * 60 , i, "Break", s+1);
                sleep(1);
            }
            sendNotification("Break completed! Get back to work!");
        }
    }
    printf("\nAll sessions completed! Well done!\n");
}

void displayProgress(int totalSeconds, int elapsedSeconds, char* label, int session) {
    printf("\033[H\033[J");
    printf("Session %d\n", session);

    int remainingTime = totalSeconds / 60 - elapsedSeconds / 60;
    printf("%s for %d minute%s.\n", label, remainingTime, remainingTime == 1 ? "" : "s");

    int remainingSeconds = totalSeconds - elapsedSeconds;
    int remainingMinutes = remainingSeconds / 60;
    remainingSeconds %= 60;
    
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    int barLength = w.ws_col - 30;
    if (barLength <0) barLength = 0;

    int completedBars = (barLength * (elapsedSeconds + 1)) / totalSeconds;
    int remainingBars = barLength - completedBars;

    printf("\r%02d:%02d %s \u2503", remainingMinutes, remainingSeconds, label);

    if (strcmp(label, "Work") == 0) {
        printf("\033[48;2;208;53;197m");
    } else {
        printf("\033[48;2;50;192;50m");
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

int main (int argc, char *argv[]) {
    enableRawMode();
    
    int workMinutes = 25;
    int breakMinutes = 5;
    int sessions = 1;

    for (int i=1; i<argc; i++) {
        if (strstr(argv[i], "-")) {
            sscanf(argv[i], "%d-%d", &workMinutes, &breakMinutes);
        } else if (strstr(argv[i], "n=")) {
            sscanf(argv[i], "n=%d", &sessions);
        }
    }

    displayTimer(workMinutes, breakMinutes, sessions);

    return 0;
}
