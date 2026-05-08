# OrinBot System Integration Files

This folder contains the systemd, startup, ROS bridge, and configuration files used to launch the OrinBot ROS2 stack on the Jetson Orin.

## Startup service

- `systemd/orin_stack.service`
- `systemd/orin_stack.service.d/override.conf`
- `scripts/orin_stack`

The `orin_stack.service` systemd unit launches `/data/orin_stack`, which sources startup scripts from `/data/orin_start.d`.

## Active startup scripts

- `05_description.sh` — robot description / robot_state_publisher
- `32_lidar_driver.sh` — Unitree LiDAR driver
- `37_tf_bridges.sh` — static TF bridge
- `38_joy_node.sh` — PS controller joy_node
- `39_motor_tf.sh` — static motor/wheel TF frames around `base_link`
- `40_joy_to_vesc_udp.sh` — ROS joystick to ESP32 UDP motor command bridge
- `41_vesc_udp_status.sh` — ESP32 UDP VESC feedback into ROS2 topics
- `60_point_lio.sh` — corrected odometry / point-LIO related startup

## Motor bridge nodes

- `joy_to_vesc_udp.py`

Publishes joystick-derived RPM commands over UDP to the ESP32 CAN bridge at `192.168.50.68:20001`.

- `vesc_udp_status_node.py`

Listens for ESP32/VESC telemetry on UDP port `20000` and publishes:

- `/vesc/debug_raw`
- `/vesc/motor_11/erpm`, `/current`, `/duty`
- `/vesc/motor_12/erpm`, `/current`, `/duty`
- `/vesc/motor_21/erpm`, `/current`, `/duty`
- `/vesc/motor_22/erpm`, `/current`, `/duty`

## Network map

- Orin Ethernet: `192.168.50.2`
- OAK4-D: `192.168.50.60`
- ESP32 CAN bridge: `192.168.50.68`
- ESP32 command listener: UDP `20001`
- ESP32 telemetry return: UDP `20000`

Large ROS bags, logs, build directories, and temporary files are intentionally excluded.
