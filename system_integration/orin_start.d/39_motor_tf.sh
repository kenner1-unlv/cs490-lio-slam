#!/usr/bin/env bash
set -e

LOGFILE="/tmp/motor_tf.log"

log() {
  echo "[orin_bringup] $(date '+%F %T') $*"
}

log "=== Starting motor/wheel static TF frames ==="

source /opt/ros/humble/setup.bash
source /data/ros_ws/install/setup.bash

# 24 inch square wheelbase/track width
# 24 in = 0.6096 m, half = 0.3048 m
# ROS convention: x forward, y left, z up

ros2 run tf2_ros static_transform_publisher \
  0.3048 0.3048 0.0 0 0 0 \
  base_link front_left_motor_12 >> "$LOGFILE" 2>&1 &
PIDS+=($!)

ros2 run tf2_ros static_transform_publisher \
  0.3048 -0.3048 0.0 0 0 0 \
  base_link front_right_motor_11 >> "$LOGFILE" 2>&1 &
PIDS+=($!)

ros2 run tf2_ros static_transform_publisher \
  -0.3048 0.3048 0.0 0 0 0 \
  base_link rear_left_motor_22 >> "$LOGFILE" 2>&1 &
PIDS+=($!)

ros2 run tf2_ros static_transform_publisher \
  -0.3048 -0.3048 0.0 0 0 0 \
  base_link rear_right_motor_21 >> "$LOGFILE" 2>&1 &
PIDS+=($!)

log "motor TF frames launched: ${PIDS[*]}"
