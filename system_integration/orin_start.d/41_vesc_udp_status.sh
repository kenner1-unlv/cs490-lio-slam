#!/usr/bin/env bash
set -e

LOGFILE="/tmp/vesc_udp_status.log"

log() {
  echo "[orin_bringup] $(date '+%F %T') $*"
}

log "=== Starting VESC UDP Status ROS2 Bridge ==="

source /opt/ros/humble/setup.bash
source /data/ros_ws/install/setup.bash

python3 /data/ros_ws/src/joystick_drivers/joystick_drivers/vesc_udp_status_node.py >> "$LOGFILE" 2>&1 &

PIDS+=($!)
log "vesc_udp_status_node launched with PID ${PIDS[-1]} → logging to $LOGFILE"
