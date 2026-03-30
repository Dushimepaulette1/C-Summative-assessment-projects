# Project 2: Linux Server Health Monitoring and Alert Automation Script

## What This Project Does
This is a Bash shell script that monitors the health of a Linux server in real time.
It tracks CPU usage, memory consumption, disk space, and active processes —
then warns you automatically when any resource exceeds the limit you set.

## How to Run

```bash
# Step 1: Make the script executable (only needed once)
chmod +x monitor.sh

# Step 2: Run the script
./monitor.sh
```

## Menu Options

| Option | Description |
|--------|-------------|
| 1 | Display current system health (CPU, RAM, Disk, Processes) |
| 2 | Configure warning thresholds (change the % limits) |
| 3 | View the activity log file with timestamps |
| 4 | Clear the log file |
| 5 | Start automatic monitoring (checks every 60 seconds) |
| 6 | Stop automatic monitoring |
| 7 | Exit the program |

## Key Features
- **System monitoring** — uses `top`, `free`, `df`, and `ps` to gather live stats
- **Threshold alerts** — shows a red warning when CPU/RAM/Disk exceeds your limit
- **Timestamped logging** — saves every check to `server_health.log`
- **Automatic monitoring** — runs in the background using a loop and `sleep`
- **Interactive menu** — clean terminal UI with color-coded output
- **Error handling** — validates all user input and checks for missing commands

## Key Concepts Used
- Bash functions
- `case` statements for menu routing
- Output redirection (`>>` to append logs)
- Background processes (`&`) with PID tracking
- Input validation using regex (`[[ $var =~ ^[0-9]+$ ]]`)
- ANSI color codes for colored terminal output
- Linux tools: `top`, `free`, `df`, `ps`, `awk`, `date`

## Files
- `monitor.sh` — the main script
- `server_health.log` — auto-generated log file (created when script runs)