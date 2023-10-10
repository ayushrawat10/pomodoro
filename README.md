# Pomodoro-Timer-CLI

A simple terminal-based Pomodoro Timer written in C. Utilize this tool to enhance your productivity by dividing your work sessions into focused intervals, separated by short breaks. It is customizable, allowing you to set the work and break periods, and choose different color schemes through a configuration file.

## Features
- **Customize Work and Break Intervals:** Adjust the duration of work and break intervals to suit your needs.
- **Multiple Sessions:** Ability to set multiple work sessions with breaks in between.
- **Real-Time Progress Bar:** A colorful, real-time progress bar that visualizes the countdown.
- **Session Count:** Determine the number of sessions before a long break.
- **Command Line Customization:** Change settings directly from the terminal.
- **Persistent Configurations:** Saves your preferred default settings in a configuration file for future sessions.
- **Notifications:** Get notified when it's time to take a break or get back to work.

## Installation

### Using Homebrew (macOS)
There's a formula available for Homebrew users. You can install the Pomodoro Timer using the following commands:

```sh
brew tap ayushrawat10/homebrew-pomodoro
brew install pomodoro
```

### Manual Installation
Clone the repository and compile the source code using the provided Makefile:
```sh
git clone git@github.com:ayushrawat10/pomodoro-timer-cli.git
cd pomodoro-timer-cli
make
```
Or,
```sh
git clone git@github.com:ayushrawat10/pomodoro-timer-cli.git
cd pomodoro-timer-cli
gcc pomodoro.c -o pomodoro
```
Then the timer can be run with:
```sh
./pomodoro
```

## Usage
### Basic Usage
Run the timer with the default settings if installed using homebrew:
```sh
pomodoro
```
Or,
Run the timer with the default settings if installed manually:
```sh
./path-to-the-file/pomodoro
```

### Customizing the Timer from Command Line
Customize work and break durations, and session count:
```sh
pomodoro 30-10 n=4
```
This command will run a session with a 30 minute work timer, 10 minute break timer for four sessions.

### Configuration File
The timer also reads settings from a configuration file located at ```~/.config/pomodoro/pomodoro_config.txt```. This file is automatically created and updated when you first configure the timer.
#### Note:
This config file will only be created after you use the config command for the first time. All the custom settings will have to be explicitly written in the pomodoro command as above.

## Customization Configuration
Customize the default values as per your needs for future sessions.
```sh
pomodoro config work=40 break=10 sessions=2 "workcolor=(208,53,197) breakcolor=(50,200,50)"
```
This command will set the future default work duration to 40 minutes, break duration to 10 minutes, and the number of sessions when not mentioned to the value 2. 'workcolor' and 'breakcolor' need to have RGB values and will change the color of the bar as it counts down to 0.

## Contribution
Contributions are welcome! Feel free to fork the repository and open a pull request. For bugs and feature requests, please create an issue.

### Happy timeboxing!
