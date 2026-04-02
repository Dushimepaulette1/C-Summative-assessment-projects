# Summative Assessment Projects

A collection of five systems programming projects covering embedded systems, Linux scripting, C programming with dynamic memory, function pointers, and multi-threaded applications.

## Video Presentation

[![Summative Project Presentation](https://img.youtube.com/vi/M0tgBpsZ-cE/0.jpg)](https://youtu.be/M0tgBpsZ-cE)

> Click the thumbnail above to watch the full project presentation on YouTube.

---

## Projects Overview

| # | Project | Language | Key Concepts |
|---|---------|----------|--------------|
| 1 | Smart Traffic Light Controller | Arduino (C++) | Structs, pointers, malloc, millis(), serial interface |
| 2 | Linux Server Health Monitor | Bash | System monitoring, logging, automation, thresholds |
| 3 | Academic Records Analyzer | C | Structs, dynamic memory, CRUD, file handling, sorting |
| 4 | Data Analysis Toolkit | C | Function pointers, callbacks, dispatch table, dynamic arrays |
| 5 | Multi-threaded Web Scraper | C | POSIX threads, libcurl, concurrent downloads |

---

## Project 1 — Smart Traffic Light Controller

**Platform:** Arduino UNO (simulated in Tinkercad)

Simulates a smart traffic light system managing two intersections simultaneously. Uses non-blocking timing with `millis()` so both intersections update concurrently without freezing the program.

**Key Features:**
- Two intersections (North-South and East-West) with Red, Yellow, and Green LEDs
- Push buttons simulate vehicle detection
- Dynamic timing — extended green time when traffic is detected
- Serial monitor interface with live status, manual overrides, and vehicle count reset
- Dynamic memory allocation using `malloc()` for intersection data

**Wiring:**

| Component | Arduino Pin |
|-----------|-------------|
| Intersection 0 Red LED | Pin 2 |
| Intersection 0 Yellow LED | Pin 3 |
| Intersection 0 Green LED | Pin 4 |
| Intersection 0 Button | Pin 5 |
| Intersection 1 Red LED | Pin 6 |
| Intersection 1 Yellow LED | Pin 7 |
| Intersection 1 Green LED | Pin 8 |
| Intersection 1 Button | Pin 9 |

**How to run:**
1. Open [Tinkercad](https://www.tinkercad.com)
2. Wire components as shown above
3. Paste `traffic_light.ino` into the code editor (Text mode)
4. Start Simulation and open the Serial Monitor
5. Type `m` in Serial Monitor for the command menu

---

## Project 2 — Linux Server Health Monitor

**Language:** Bash Shell Script

An automated server health monitoring script that tracks CPU, memory, disk usage, and active processes. Triggers alerts when usage exceeds configurable thresholds and logs all activity with timestamps.

**Key Features:**
- Real-time monitoring of CPU, memory, disk, and processes
- Configurable alert thresholds (default: CPU 80%, Memory 85%, Disk 90%)
- Timestamped logging to `server_health.log`
- Automatic background monitoring every 60 seconds
- Interactive menu with 7 options
- Color-coded terminal output

**How to run:**
```bash
chmod +x monitor.sh
./monitor.sh
```

**Menu Options:**
1. Display current system health
2. Configure monitoring thresholds
3. View activity logs
4. Clear logs
5. Start automatic monitoring
6. Stop automatic monitoring
7. Exit

---

## Project 3 — Academic Records Analyzer

**Language:** C

A full academic records management system that stores student data using structures and dynamic memory. Supports complete CRUD operations, manual search and sorting algorithms, and detailed performance reports.

**Key Features:**
- Student records stored in C structures (ID, name, course, age, grades, GPA)
- Dynamic array using `malloc()` and `realloc()` — no fixed size limit
- Binary file persistence — records saved and loaded between sessions
- Search by ID or name (partial, case-insensitive)
- Bubble sort by GPA, name, or ID
- Performance reports: class average, highest/lowest GPA, median, top 3 students, and course-based analytics

**How to compile and run:**
```bash
gcc -o academic_analyzer project3_academic.c
./academic_analyzer
```

---

## Project 4 — Data Analysis Toolkit

**Language:** C

A data analysis toolkit that demonstrates function pointers and callback-based processing. Operations are dispatched through a function pointer table — no long if-else chains.

**Key Features:**
- Function pointer dispatch table maps menu choices directly to functions
- Callback functions for filtering, transformations, and sorting
- Supported operations: sum, average, min, max, standard deviation, search, filter, scale, shift, sort
- Dynamic dataset using `malloc()` and `realloc()`
- File load and save for datasets

**How to compile and run:**
```bash
gcc -o toolkit project4_toolkit.c -lm
./toolkit
```

---

## Project 5 — Multi-threaded Web Scraper

**Language:** C (POSIX threads + libcurl)

A multi-threaded web scraper that fetches multiple URLs simultaneously using pthreads. Each thread downloads one URL and saves the content to its own output file.

**Key Features:**
- Parallel downloads using POSIX threads (`pthread_create`, `pthread_join`)
- Each thread saves output to a separate file (`scraped_output/output_N.txt`)
- libcurl write callback for chunked data collection
- Graceful error handling for unreachable URLs
- Summary report after all threads complete
- Supports loading custom URL lists from a text file

**Dependencies:**
```bash
sudo apt-get install libcurl4-openssl-dev
```

**How to compile and run:**
```bash
gcc -o scraper project5_scraper.c -lpthread -lcurl
./scraper
# or with a custom URL list:
./scraper urls.txt
```

---

## Repository Structure

```
C-Summative-assessment-projects/
├── project1-traffic-light/
│   └── traffic_light.ino
├── project2-health-monitor/
│   ├── monitor.sh
│   └── server_health.log
├── project3-academic-records/
│   ├── project3_academic.c
│   └── students.dat
├── project4-data-toolkit/
│   └── project4_toolkit.c
└── project5-web-scraper/
    ├── project5_scraper.c
    └── scraped_output/
```

---

## Author

**Dushimepaulette** — Summative Assessment 2026
