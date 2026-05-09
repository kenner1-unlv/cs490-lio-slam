# RDK X5 ROS2 Sensor Bridge Stack

This folder documents the ROS2 sensor bridge work running on the RDK X5.

The RDK X5 acts as a distributed sensor node for Orinbot. It handles local sensor wiring and publishes clean ROS2 topics that can be consumed by the Jetson Orin.

## Current bridges

### GPS HAT bridge

Hardware:

- Waveshare / Quectel LC29H GPS/RTK HAT
- Mounted directly on the RDK X5 40-pin Raspberry Pi-style header
- HAT jumper set to Pi/RDK UART mode
- GPS antenna connected

Confirmed serial path:

- Device: `/dev/ttyS1`
- Baud: `115200`
- Input: NMEA
- Output topic: `/gps/fix`
- Message type: `sensor_msgs/msg/NavSatFix`
- Frame: `gps_link`

The node parses NMEA GGA messages and publishes ROS2 GPS fixes. Position covariance is approximated from HDOP.

Privacy note: real latitude and longitude values are intentionally not documented.

### ToF / Ultrasonic range bridge

The range bridge handles short-range sensors connected to the RDK X5 and publishes ROS2 range-style data for obstacle awareness, debugging, and future safety behavior.

Preserved files include:

- `sensor_bridges/range_bridge/tof_bridge.py`
- `legacy_start_scripts/20_tof_bridge.sh`
- `log_examples/20_tof_bridge.log`
- `log_examples/demo_tof.log`

## Systemd services

Services copied from the RDK:

- `systemd/rdk_gps_bridge.service`
- `systemd/rdk_range_bridge.service`
- `systemd/rdk_stack.service.OFF`

The GPS service starts on boot, sources ROS2 Humble, reads `/dev/ttyS1`, and publishes `/gps/fix`.

## Useful commands

Check GPS serial directly:

    sudo stty -F /dev/ttyS1 115200 raw -echo
    timeout 5 cat /dev/ttyS1

Check GPS service:

    systemctl status rdk_gps_bridge.service --no-pager -l
    journalctl -u rdk_gps_bridge.service -f

Check GPS ROS2 topic:

    source /opt/ros/humble/setup.bash
    export RMW_IMPLEMENTATION=rmw_fastrtps_cpp
    export ROS_DOMAIN_ID=0
    ros2 topic echo /gps/fix

Check range bridge service:

    systemctl status rdk_range_bridge.service --no-pager -l
    journalctl -u rdk_range_bridge.service -f

## Strategy

Sensor flow:

    GPS HAT / ToF / Ultrasonic sensors
        -> RDK X5 local ROS2 bridge nodes
        -> ROS2 topics
        -> Jetson Orin logging / fusion / autonomy stack

This keeps sensor bring-up modular. The Orin does not need to directly own every serial device, GPIO, or sensor wire.
