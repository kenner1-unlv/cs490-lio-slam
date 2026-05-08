#!/usr/bin/env bash
set -euo pipefail

echo "[tf_bridges] publishing map/base bridge"

ros2 run tf2_ros static_transform_publisher \
  --x 0.0 --y 0.0 --z 0.0 \
  --roll 0.0 --pitch 0.0 --yaw 0.0 \
  --frame-id camera_init \
  --child-frame-id base_link &

# no wait here; allow orin_stack to continue to joy and UDP bridge
