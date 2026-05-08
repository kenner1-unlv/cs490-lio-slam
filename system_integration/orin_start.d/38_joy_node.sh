#!/usr/bin/env bash
set -eo pipefail

LOGFILE="/tmp/joy_node.log"

log() { echo "[orin_bringup] $(date +'%F %T') $*" | tee -a "$LOGFILE" ; }

log "=== Starting joy_node (PS5 controller driver) ==="

# Source ROS environment
set +u
if [ -f /opt/ros/humble/setup.bash ]; then
  source /opt/ros/humble/setup.bash
fi
if [ -f /data/ros_ws/install/setup.bash ]; then
  source /data/ros_ws/install/setup.bash
fi
set -u

# Start joy_node quietly in background
ros2 run joy joy_node >> "$LOGFILE" 2>&1 &

PIDS+=("$!")
log "joy_node launched with PID ${PIDS[-1]} → logging to $LOGFILE"
