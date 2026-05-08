#!/usr/bin/env bash
set -euo pipefail

echo "[34_point_lio] starting Point-LIO corrected odometry..."

source /opt/ros/humble/setup.bash
source /data/ros_ws/install/setup.bash

export ROS_DOMAIN_ID=${ROS_DOMAIN_ID:-0}
export RMW_IMPLEMENTATION=${RMW_IMPLEMENTATION:-rmw_fastrtps_cpp}

# Give Unitree driver + robot_description/TF time to come up.
sleep 8

# Wait briefly for required topics so Point-LIO does not start blind.
for topic in /unilidar/cloud /unilidar/imu; do
  echo "[34_point_lio] waiting for $topic ..."
  for i in $(seq 1 20); do
    if ros2 topic list | grep -qx "$topic"; then
      echo "[34_point_lio] found $topic"
      break
    fi
    sleep 1
  done
done

exec ros2 launch point_lio correct_odom_unilidar_l2.launch.py
