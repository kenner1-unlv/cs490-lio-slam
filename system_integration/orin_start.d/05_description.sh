#!/usr/bin/env bash
set -euo pipefail

log "Starting orinbot_description (robot_state_publisher)..."

if [[ -z "${ROS_DISTRO:-}" ]]; then
  source /opt/ros/humble/setup.bash
  if [[ -f /data/ros_ws/install/setup.bash ]]; then
    source /data/ros_ws/install/setup.bash
  fi
fi

ros2 launch orinbot_description description.launch.py &

PIDS+=("$!")
log "orinbot_description launched with PID ${PIDS[-1]}"
