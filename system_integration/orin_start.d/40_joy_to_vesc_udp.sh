#!/usr/bin/env bash
set -eo pipefail

LOGFILE="/tmp/joy_to_vesc_udp.log"

log() { echo "[orin_bringup] $(date +'%F %T') $*" | tee -a "$LOGFILE" ; }

log "=== Starting Joy to VESC UDP Bridge ==="

# Source ROS
set +u
if [ -f /opt/ros/humble/setup.bash ]; then
  source /opt/ros/humble/setup.bash
fi
if [ -f /data/ros_ws/install/setup.bash ]; then
  source /data/ros_ws/install/setup.bash
fi
set -u

# Start node quietly (all output goes to log file)
python3 /data/ros_ws/src/joystick_drivers/joystick_drivers/joy_to_vesc_udp.py >> "$LOGFILE" 2>&1 &

PIDS+=("$!")
log "joy_to_vesc_udp launched with PID ${PIDS[-1]} → logging to $LOGFILE"
