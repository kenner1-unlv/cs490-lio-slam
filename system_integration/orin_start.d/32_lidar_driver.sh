#!/usr/bin/env bash
set -euo pipefail

source /opt/ros/humble/setup.bash
source /data/ros_ws/install/setup.bash

# This file is sourced by /data/orin_stack (systemd service).
# It assumes ROS + overlays have already been sourced.

# allow running under orin.bringup (no log() defined)
if ! command -v log >/dev/null 2>&1; then
  log() { echo "[orin_start.d] $*"; }
fi

# if PIDS isn't defined by the parent script, define it so "PIDS+=()" won't crash under -u
: "${PIDS:=()}"

log "[lidar] starting unitree_lidar_ros2 via launch..."

# Fixed: no spaces around =
LAUNCH_CMD="ros2 launch unitree_lidar_ros2 launch.py"

# Launch in background and capture PID
$LAUNCH_CMD &
PIDS+=("$!")

log "[lidar] unitree launch pid=${PIDS[-1]}"

# ---- TF bridge: connect Unitree TF tree to robot TF tree ----
# Unitree publishes: unilidar_imu_initial -> unilidar_imu -> unilidar_lidar_initial (dynamic TF)
# URDF publishes: base_link -> ... -> unilidar_lidar (static TF)
# This static TF makes everything one connected TF tree (identity for now).
# (your TF static publisher code can go here if not launched elsewhere)
