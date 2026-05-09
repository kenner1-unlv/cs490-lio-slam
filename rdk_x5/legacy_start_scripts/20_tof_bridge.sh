#!/usr/bin/env bash
set -o pipefail

echo "[tof_bridge] checking serial sensors"

if [ ! -e /dev/ttyUSB0 ] || [ ! -e /dev/ttyUSB1 ]; then
  echo "[tof_bridge] WARN: expected /dev/ttyUSB0 and /dev/ttyUSB1, found:"
  ls -lah /dev/ttyUSB* /dev/ttyACM* 2>/dev/null || true
  exit 0
fi

source /opt/ros/humble/setup.bash
export ROS_DOMAIN_ID=0
export RMW_IMPLEMENTATION=rmw_fastrtps_cpp

echo "[tof_bridge] starting"
exec python3 /home/sunrise/tof_bridge.py
