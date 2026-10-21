# Study Schedule — C++17

A lightweight terminal-based student timetable manager written in standard C++17. This package includes the supplied `main.cpp` plus the missing UI/model header and build script so it can compile as a complete project.

## Features
- Add, edit, and delete classes or free-time windows
- Class name, room, class type (lecture/practice/remote), start and end times
- Chronological sorting and overlap warning/confirmation
- Navigate days, jump to a day offset or ISO date
- 30-day planning range (today through 29 days ahead); keeps the previous 7 days and automatically purges older dates
- Name/time suggestions based on existing schedule entries
- Five terminal colour palettes
- Widget-style “Up Next” terminal view
- Saves locally; no account or internet connection required

## Build and run (Linux, macOS, or a C++17 environment)

```sh
chmod +x build.sh
./build.sh
./studyschedule
```

Widget view:

```sh
./studyschedule --widget
```

The app stores data in `~/.study_schedule.dat` (or `study_schedule.dat` if `HOME` is unavailable).

## Windows

With MinGW-w64 installed, run from the project folder:

```bat
g++ -std=c++17 -Wall -Wextra -O2 main.cpp -o studyschedule.exe
studyschedule.exe
```

## Important platform note

This is a **C++ terminal app**, not a native Android APK and not a graphical Android home-screen widget. On Android, a terminal/IDE that supports compiling C++ can run the console version; a native Android interface or system home-screen widget would need an Android project (typically Kotlin/Java, optionally using C++ through the NDK).
