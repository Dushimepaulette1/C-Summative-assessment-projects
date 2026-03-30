#!/bin/bash
# ============================================================
#  PROJECT 2: Linux Server Health Monitoring Script
#  Language : Bash (Shell Script)
#  Author   : [Your Name]
# ============================================================
#
#  HOW IT WORKS:
#  - Reads CPU, Memory, Disk, and Process info from the system
#  - Compares them against limits you set (thresholds)
#  - Shows warnings if anything is too high
#  - Saves everything to a log file with timestamps
#  - Runs automatically in a loop every 60 seconds
#  - Has an interactive menu so you can control it easily
#
#  HOW TO RUN:
#  1. Save this file as monitor.sh
#  2. Make it executable: chmod +x monitor.sh
#  3. Run it: ./monitor.sh
# ============================================================

# ── Configuration (Change these values as you wish) ──────────
LOG_FILE="server_health.log"    # Where logs are saved
CHECK_INTERVAL=60               # Seconds between auto-checks

# Default thresholds (percentage limits)
CPU_THRESHOLD=80      # Warn if CPU usage > 80%
MEM_THRESHOLD=85      # Warn if Memory usage > 85%
DISK_THRESHOLD=90     # Warn if Disk usage > 90%

# ── Colors for prettier terminal output ──────────────────────
# These are ANSI escape codes that colorize text
RED='\033[0;31m'       # Red  → for errors/warnings
GREEN='\033[0;32m'     # Green → for OK status
YELLOW='\033[1;33m'    # Yellow → for mild warnings
BLUE='\033[0;34m'      # Blue → for info/headers
NC='\033[0m'           # No Color → resets color back to normal

# ── Global flag to control monitoring loop ───────────────────
MONITORING_ACTIVE=false   # Start as inactive (user must start it)

# =============================================================
#  FUNCTION: print_header
#  Purpose : Print a nice section header
#  Usage   : print_header "Section Title"
# =============================================================
print_header() {
  echo ""
  echo -e "${BLUE}============================================${NC}"
  echo -e "${BLUE}  $1${NC}"
  echo -e "${BLUE}============================================${NC}"
}

# =============================================================
#  FUNCTION: print_separator
#  Purpose : Print a simple divider line
# =============================================================
print_separator() {
  echo "--------------------------------------------"
}

# =============================================================
#  FUNCTION: get_timestamp
#  Purpose : Return current date and time as a string
#  Example output: [2025-06-10 14:35:22]
# =============================================================
get_timestamp() {
  date "+[%Y-%m-%d %H:%M:%S]"
}

# =============================================================
#  FUNCTION: log_message
#  Purpose : Write a message to BOTH the screen AND the log file
#  Usage   : log_message "INFO" "Everything is OK"
# =============================================================
log_message() {
  local level="$1"    # e.g., INFO, WARNING, ALERT
  local message="$2"  # The actual message text

  # Build the full log line with timestamp
  local log_line="$(get_timestamp) [$level] $message"

  # Print to terminal
  echo "$log_line"

  # Also save to log file (>> means APPEND, not overwrite)
  echo "$log_line" >> "$LOG_FILE"
}

# =============================================================
#  FUNCTION: get_cpu_usage
#  Purpose : Get the current CPU usage as a whole number (%)
#  Returns : A number like 45 (meaning 45%)
#
#  HOW IT WORKS:
#  - 'top -bn1' runs 'top' once (-n1) in batch mode (-b)
#  - We search for the line with "Cpu(s)"
#  - awk extracts the idle % and we calculate: 100 - idle = used
# =============================================================
get_cpu_usage() {
  # Read CPU stats from /proc/stat — works reliably in foreground AND background
  # /proc/stat line 1 looks like: cpu  264 0 294 15348 12 0 4 0 0 0
  # Fields: user nice system idle iowait irq softirq ...
  # Total = sum of all fields. Used = Total - idle. Usage% = Used/Total * 100

  local cpu_line
  cpu_line=$(grep '^cpu ' /proc/stat)

  local user nice system idle iowait irq softirq
  read -r _ user nice system idle iowait irq softirq _ <<< "$cpu_line"

  local total=$(( user + nice + system + idle + iowait + irq + softirq ))
  local used=$(( total - idle ))

  # Avoid divide-by-zero
  if [[ "$total" -eq 0 ]]; then
    echo 0
    return
  fi

  # Calculate percentage as integer
  echo $(( used * 100 / total ))
}

# =============================================================
#  FUNCTION: get_memory_usage
#  Purpose : Get RAM usage as a percentage
#  Returns : A number like 70 (meaning 70% of RAM is used)
#
#  HOW IT WORKS:
#  - 'free -m' shows memory in megabytes
#  - awk reads the "Mem:" line and calculates used/total * 100
# =============================================================
get_memory_usage() {
  free -m | awk '/Mem:/ {
    total = $2
    used  = $3
    if (total == 0) print 0
    else printf "%d", (used / total) * 100
  }'
}

# =============================================================
#  FUNCTION: get_disk_usage
#  Purpose : Get the disk usage of the root partition (/)
#  Returns : A number like 55 (meaning 55% of disk is used)
#
#  HOW IT WORKS:
#  - 'df -h /' shows disk usage for the root filesystem
#  - awk grabs the 5th column (the % value) and strips the '%' sign
# =============================================================
get_disk_usage() {
  df -h / | awk 'NR==2 {gsub("%", "", $5); print $5}'
}

# =============================================================
#  FUNCTION: get_process_count
#  Purpose : Count how many processes are currently running
# =============================================================
get_process_count() {
  ps aux | wc -l
}

# =============================================================
#  FUNCTION: get_top_processes
#  Purpose : Show the top 5 processes using the most CPU
# =============================================================
get_top_processes() {
  echo "Top 5 Processes by CPU usage:"
  print_separator
  # ps: list processes | sort by CPU (column 3) in reverse | show top 5
  ps aux --sort=-%cpu | awk 'NR<=6 {printf "%-20s %-10s %-10s\n", $11, $3"%", $4"%"}' \
    | column -t
}

# =============================================================
#  FUNCTION: check_threshold
#  Purpose : Compare a value against a threshold and warn if over
#  Usage   : check_threshold "CPU" 85 80
#  Params  : resource_name, current_value, threshold_value
# =============================================================
check_threshold() {
  local resource="$1"   # e.g., "CPU"
  local current="$2"    # e.g., 85
  local threshold="$3"  # e.g., 80

  if (( current > threshold )); then
    # Over the limit → show red warning and log it
    echo -e "${RED}⚠ WARNING: $resource usage is ${current}% (Limit: ${threshold}%)${NC}"
    log_message "ALERT" "$resource usage is ${current}% — exceeded threshold of ${threshold}%"
    return 1  # Return 1 = problem found
  else
    # Under the limit → show green OK
    echo -e "${GREEN}✓ $resource: ${current}% (Limit: ${threshold}%) — OK${NC}"
    log_message "INFO" "$resource usage is ${current}% — within safe limits"
    return 0  # Return 0 = all good
  fi
}

# =============================================================
#  FUNCTION: display_health
#  Purpose : Collect all metrics and display them on screen
# =============================================================
display_health() {
  print_header "SYSTEM HEALTH REPORT"

  # Collect all metrics
  local cpu_usage
  local mem_usage
  local disk_usage
  local proc_count

  cpu_usage=$(get_cpu_usage)
  mem_usage=$(get_memory_usage)
  disk_usage=$(get_disk_usage)
  proc_count=$(get_process_count)

  echo ""
  echo -e "${YELLOW}📊 Current Metrics:${NC}"
  print_separator

  echo -e "  CPU Usage    : ${cpu_usage}%"
  echo -e "  Memory Usage : ${mem_usage}%"
  echo -e "  Disk Usage   : ${disk_usage}%"
  echo -e "  Active Procs : ${proc_count}"
  echo -e "  Time Checked : $(get_timestamp)"

  echo ""
  echo -e "${YELLOW}🔍 Threshold Check:${NC}"
  print_separator

  # Check each metric against its threshold
  check_threshold "CPU"    "$cpu_usage"  "$CPU_THRESHOLD"
  check_threshold "Memory" "$mem_usage"  "$MEM_THRESHOLD"
  check_threshold "Disk"   "$disk_usage" "$DISK_THRESHOLD"

  echo ""
  # Show top processes
  get_top_processes

  echo ""
  log_message "INFO" "Health check complete — CPU:${cpu_usage}% MEM:${mem_usage}% DISK:${disk_usage}%"
}

# =============================================================
#  FUNCTION: configure_thresholds
#  Purpose : Allow user to change the warning limits interactively
# =============================================================
configure_thresholds() {
  print_header "CONFIGURE THRESHOLDS"
  echo "Current limits:"
  echo "  CPU  Threshold : ${CPU_THRESHOLD}%"
  echo "  MEM  Threshold : ${MEM_THRESHOLD}%"
  echo "  DISK Threshold : ${DISK_THRESHOLD}%"
  echo ""

  # Read new CPU threshold from user
  read -rp "Enter new CPU threshold (1-100) [press Enter to keep ${CPU_THRESHOLD}]: " new_cpu
  if [[ -n "$new_cpu" ]]; then
    # Validate: must be a number between 1 and 100
    if [[ "$new_cpu" =~ ^[0-9]+$ ]] && (( new_cpu >= 1 && new_cpu <= 100 )); then
      CPU_THRESHOLD="$new_cpu"
      echo -e "${GREEN}CPU threshold updated to ${CPU_THRESHOLD}%${NC}"
    else
      echo -e "${RED}Invalid input. Keeping old value: ${CPU_THRESHOLD}%${NC}"
    fi
  fi

  # Read new Memory threshold
  read -rp "Enter new Memory threshold (1-100) [press Enter to keep ${MEM_THRESHOLD}]: " new_mem
  if [[ -n "$new_mem" ]]; then
    if [[ "$new_mem" =~ ^[0-9]+$ ]] && (( new_mem >= 1 && new_mem <= 100 )); then
      MEM_THRESHOLD="$new_mem"
      echo -e "${GREEN}Memory threshold updated to ${MEM_THRESHOLD}%${NC}"
    else
      echo -e "${RED}Invalid input. Keeping old value: ${MEM_THRESHOLD}%${NC}"
    fi
  fi

  # Read new Disk threshold
  read -rp "Enter new Disk threshold (1-100) [press Enter to keep ${DISK_THRESHOLD}]: " new_disk
  if [[ -n "$new_disk" ]]; then
    if [[ "$new_disk" =~ ^[0-9]+$ ]] && (( new_disk >= 1 && new_disk <= 100 )); then
      DISK_THRESHOLD="$new_disk"
      echo -e "${GREEN}Disk threshold updated to ${DISK_THRESHOLD}%${NC}"
    else
      echo -e "${RED}Invalid input. Keeping old value: ${DISK_THRESHOLD}%${NC}"
    fi
  fi

  log_message "CONFIG" "Thresholds updated — CPU:${CPU_THRESHOLD}% MEM:${MEM_THRESHOLD}% DISK:${DISK_THRESHOLD}%"
}

# =============================================================
#  FUNCTION: view_logs
#  Purpose : Display the contents of the log file
# =============================================================
view_logs() {
  print_header "ACTIVITY LOG"

  # Check if log file exists
  if [[ ! -f "$LOG_FILE" ]]; then
    echo -e "${YELLOW}No log file found yet. Run a health check first.${NC}"
    return
  fi

  echo "Log file: $LOG_FILE"
  echo "Total entries: $(wc -l < "$LOG_FILE")"
  print_separator
  cat "$LOG_FILE"  # Show entire log file
}

# =============================================================
#  FUNCTION: clear_logs
#  Purpose : Delete the log file contents after confirming
# =============================================================
clear_logs() {
  print_header "CLEAR LOGS"

  read -rp "Are you sure you want to delete all logs? (yes/no): " confirm

  if [[ "$confirm" == "yes" ]]; then
    # Overwrite the log file with empty content
    > "$LOG_FILE"
    echo -e "${GREEN}Log file cleared successfully.${NC}"
    log_message "INFO" "Log file was cleared by user"
  else
    echo "Log clearing cancelled."
  fi
}

# =============================================================
#  FUNCTION: start_monitoring
#  Purpose : Run health checks automatically every N seconds
#  NOTE    : Runs in the BACKGROUND so menu stays available
# =============================================================
start_monitoring() {
  if [[ "$MONITORING_ACTIVE" == "true" ]]; then
    echo -e "${YELLOW}Monitoring is already running.${NC}"
    return
  fi

  MONITORING_ACTIVE=true
  echo -e "${GREEN}✓ Automatic monitoring started (every ${CHECK_INTERVAL} seconds).${NC}"
  echo "  Type option 6 to stop it."
  log_message "INFO" "Automatic monitoring started with interval: ${CHECK_INTERVAL}s"

  # Run monitoring loop in the background using &
  # The loop checks health, waits, then repeats until stopped
  (
    while [[ "$MONITORING_ACTIVE" == "true" ]]; do
      echo ""
      echo -e "${BLUE}[AUTO-CHECK] Running scheduled health check...${NC}"
      display_health
      sleep "$CHECK_INTERVAL"
    done
  ) &

  # Save the background process ID so we can stop it later
  MONITOR_PID=$!
}

# =============================================================
#  FUNCTION: stop_monitoring
#  Purpose : Stop the background monitoring loop
# =============================================================
stop_monitoring() {
  if [[ "$MONITORING_ACTIVE" == "false" ]]; then
    echo -e "${YELLOW}Monitoring is not currently running.${NC}"
    return
  fi

  MONITORING_ACTIVE=false

  # Kill the background process if we have its PID
  if [[ -n "$MONITOR_PID" ]]; then
    kill "$MONITOR_PID" 2>/dev/null
  fi

  echo -e "${GREEN}✓ Monitoring stopped.${NC}"
  log_message "INFO" "Automatic monitoring stopped by user"
}

# =============================================================
#  FUNCTION: show_menu
#  Purpose : Display the main interactive menu
# =============================================================
show_menu() {
  print_header "SERVER HEALTH MONITOR - MAIN MENU"
  echo "  1. Display current system health"
  echo "  2. Configure monitoring thresholds"
  echo "  3. View activity logs"
  echo "  4. Clear logs"
  echo "  5. Start automatic monitoring"
  echo "  6. Stop automatic monitoring"
  echo "  7. Exit"
  print_separator
  echo -e -n "Enter your choice [1-7]: "
}

# =============================================================
#  FUNCTION: check_dependencies
#  Purpose : Make sure required commands exist on this system
# =============================================================
check_dependencies() {
  local missing=0
  for cmd in top free df ps awk; do
    if ! command -v "$cmd" &>/dev/null; then
      echo -e "${RED}[ERROR] Required command not found: $cmd${NC}"
      missing=1
    fi
  done

  if (( missing == 1 )); then
    echo "Please install missing tools before running this script."
    exit 1
  fi
}

# =============================================================
#  MAIN PROGRAM – Starts here
# =============================================================

# First, make sure all needed tools are available
check_dependencies

# Create log file if it doesn't exist yet
touch "$LOG_FILE"
log_message "INFO" "Server Health Monitor started"

echo -e "${GREEN}Welcome to the Linux Server Health Monitor!${NC}"

# Main loop – keeps showing the menu until user chooses Exit
while true; do
  show_menu
  read -r choice   # Read user's menu choice

  # Decide what to do based on the choice
  case "$choice" in
    1) display_health ;;
    2) configure_thresholds ;;
    3) view_logs ;;
    4) clear_logs ;;
    5) start_monitoring ;;
    6) stop_monitoring ;;
    7)
      stop_monitoring  # Stop background process if running
      log_message "INFO" "Monitor exited by user"
      echo -e "${GREEN}Goodbye! Monitoring stopped.${NC}"
      exit 0
      ;;
    *)
      # User typed something invalid
      echo -e "${RED}Invalid choice '$choice'. Please enter a number between 1 and 7.${NC}"
      ;;
  esac

  # Pause so user can read the output before menu reappears
  echo ""
  read -rp "Press Enter to return to menu..."
done