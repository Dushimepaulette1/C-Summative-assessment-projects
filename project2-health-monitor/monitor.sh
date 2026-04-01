LOG_FILE="server_health.log"
CHECK_INTERVAL=60       

CPU_THRESHOLD=80        
MEM_THRESHOLD=85        
DISK_THRESHOLD=90       
# Terminal colors 
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m'            # Reset color

MONITORING_ACTIVE=false

#  Helper display functions 
print_header() {
  echo ""
  echo -e "${BLUE}============================================${NC}"
  echo -e "${BLUE}  $1${NC}"
  echo -e "${BLUE}============================================${NC}"
}

print_separator() {
  echo "--------------------------------------------"
}

get_timestamp() {
  date "+[%Y-%m-%d %H:%M:%S]"
}

log_message() {
  local level="$1"
  local message="$2"
  local log_line="$(get_timestamp) [$level] $message"
  echo "$log_line"
  echo "$log_line" >> "$LOG_FILE"
}


get_cpu_usage() {
  local cpu_line
  cpu_line=$(grep '^cpu ' /proc/stat)

  local user nice system idle iowait irq softirq
  read -r _ user nice system idle iowait irq softirq _ <<< "$cpu_line"

  local total=$(( user + nice + system + idle + iowait + irq + softirq ))
  local used=$(( total - idle ))

  if [[ "$total" -eq 0 ]]; then
    echo 0
    return
  fi

  echo $(( used * 100 / total ))
}

get_memory_usage() {
  free -m | awk '/Mem:/ {
    total = $2; used = $3
    if (total == 0) print 0
    else printf "%d", (used / total) * 100
  }'
}

get_disk_usage() {
  df -h / | awk 'NR==2 {gsub("%", "", $5); print $5}'
}

get_process_count() {
  ps aux | wc -l
}

get_top_processes() {
  echo "Top 5 Processes by CPU usage:"
  print_separator
  ps aux --sort=-%cpu | awk 'NR<=6 {printf "%-20s %-10s %-10s\n", $11, $3"%", $4"%"}' \
    | column -t
}

check_threshold() {
  local resource="$1"
  local current="$2"
  local threshold="$3"

  if (( current > threshold )); then
    echo -e "${RED}⚠ WARNING: $resource usage is ${current}% (Limit: ${threshold}%)${NC}"
    log_message "ALERT" "$resource usage is ${current}% — exceeded threshold of ${threshold}%"
    return 1
  else
    echo -e "${GREEN}✓ $resource: ${current}% (Limit: ${threshold}%) — OK${NC}"
    log_message "INFO" "$resource usage is ${current}% — within safe limits"
    return 0
  fi
}

display_health() {
  print_header "SYSTEM HEALTH REPORT"

  local cpu_usage mem_usage disk_usage proc_count
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
  check_threshold "CPU"    "$cpu_usage"  "$CPU_THRESHOLD"
  check_threshold "Memory" "$mem_usage"  "$MEM_THRESHOLD"
  check_threshold "Disk"   "$disk_usage" "$DISK_THRESHOLD"

  echo ""
  get_top_processes

  echo ""
  log_message "INFO" "Health check complete — CPU:${cpu_usage}% MEM:${mem_usage}% DISK:${disk_usage}%"
}

configure_thresholds() {
  print_header "CONFIGURE THRESHOLDS"
  echo "Current limits:"
  echo "  CPU  Threshold : ${CPU_THRESHOLD}%"
  echo "  MEM  Threshold : ${MEM_THRESHOLD}%"
  echo "  DISK Threshold : ${DISK_THRESHOLD}%"
  echo ""

  read -rp "Enter new CPU threshold (1-100) [press Enter to keep ${CPU_THRESHOLD}]: " new_cpu
  if [[ -n "$new_cpu" ]]; then
    if [[ "$new_cpu" =~ ^[0-9]+$ ]] && (( new_cpu >= 1 && new_cpu <= 100 )); then
      CPU_THRESHOLD="$new_cpu"
      echo -e "${GREEN}CPU threshold updated to ${CPU_THRESHOLD}%${NC}"
    else
      echo -e "${RED}Invalid input. Keeping old value: ${CPU_THRESHOLD}%${NC}"
    fi
  fi

  read -rp "Enter new Memory threshold (1-100) [press Enter to keep ${MEM_THRESHOLD}]: " new_mem
  if [[ -n "$new_mem" ]]; then
    if [[ "$new_mem" =~ ^[0-9]+$ ]] && (( new_mem >= 1 && new_mem <= 100 )); then
      MEM_THRESHOLD="$new_mem"
      echo -e "${GREEN}Memory threshold updated to ${MEM_THRESHOLD}%${NC}"
    else
      echo -e "${RED}Invalid input. Keeping old value: ${MEM_THRESHOLD}%${NC}"
    fi
  fi

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

view_logs() {
  print_header "ACTIVITY LOG"

  if [[ ! -f "$LOG_FILE" ]]; then
    echo -e "${YELLOW}No log file found yet. Run a health check first.${NC}"
    return
  fi

  echo "Log file: $LOG_FILE"
  echo "Total entries: $(wc -l < "$LOG_FILE")"
  print_separator
  cat "$LOG_FILE"
}

clear_logs() {
  print_header "CLEAR LOGS"

  read -rp "Are you sure you want to delete all logs? (yes/no): " confirm

  if [[ "$confirm" == "yes" ]]; then
    > "$LOG_FILE"
    echo -e "${GREEN}Log file cleared successfully.${NC}"
    log_message "INFO" "Log file was cleared by user"
  else
    echo "Log clearing cancelled."
  fi
}

start_monitoring() {
  if [[ "$MONITORING_ACTIVE" == "true" ]]; then
    echo -e "${YELLOW}Monitoring is already running.${NC}"
    return
  fi

  MONITORING_ACTIVE=true
  echo -e "${GREEN}✓ Automatic monitoring started (every ${CHECK_INTERVAL} seconds).${NC}"
  echo "  Type option 6 to stop it."
  log_message "INFO" "Automatic monitoring started with interval: ${CHECK_INTERVAL}s"

  (
    while [[ "$MONITORING_ACTIVE" == "true" ]]; do
      echo ""
      echo -e "${BLUE}[AUTO-CHECK] Running scheduled health check...${NC}"
      display_health
      sleep "$CHECK_INTERVAL"
    done
  ) &

  MONITOR_PID=$!
}

stop_monitoring() {
  if [[ "$MONITORING_ACTIVE" == "false" ]]; then
    echo -e "${YELLOW}Monitoring is not currently running.${NC}"
    return
  fi

  MONITORING_ACTIVE=false

  if [[ -n "$MONITOR_PID" ]]; then
    kill "$MONITOR_PID" 2>/dev/null
  fi

  echo -e "${GREEN}✓ Monitoring stopped.${NC}"
  log_message "INFO" "Automatic monitoring stopped by user"
}

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


check_dependencies

touch "$LOG_FILE"
log_message "INFO" "Server Health Monitor started"

echo -e "${GREEN}Welcome to the Linux Server Health Monitor!${NC}"

while true; do
  show_menu
  read -r choice

  case "$choice" in
    1) display_health ;;
    2) configure_thresholds ;;
    3) view_logs ;;
    4) clear_logs ;;
    5) start_monitoring ;;
    6) stop_monitoring ;;
    7)
      stop_monitoring
      log_message "INFO" "Monitor exited by user"
      echo -e "${GREEN}Goodbye! Monitoring stopped.${NC}"
      exit 0
      ;;
    *)
      echo -e "${RED}Invalid choice '$choice'. Please enter a number between 1 and 7.${NC}"
      ;;
  esac

  echo ""
  read -rp "Press Enter to return to menu..."
done
