#!/usr/bin/env python3

import re
import serial
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Range

BAUD = 115200

# swapped mapping:
# ttyUSB0 is right, ttyUSB1 is left
SENSORS = [
    {
        "port": "/dev/ttyUSB0",
        "frame": "tof_front_left",
    },
    {
        "port": "/dev/ttyUSB1",
        "frame": "tof_front_right",
    },
]

class ToFBridge(Node):
    def __init__(self):
        super().__init__("tof_bridge")

        self.regex = re.compile(
            r'us_cm=([0-9.\-]+).*us_ok=([01]).*tof_cm=([0-9.\-]+).*tof_ok=([01])'
        )

        self.sensors = []

        for cfg in SENSORS:
            port = cfg["port"]
            frame = cfg["frame"]

            ser = serial.Serial(port, BAUD, timeout=0.01)

            self.sensors.append({
                "port": port,
                "frame": frame,
                "ser": ser,
                "pub_tof": self.create_publisher(Range, f"/{frame}/tof", 10),
                "pub_us": self.create_publisher(Range, f"/{frame}/ultrasonic", 10),
            })

            self.get_logger().info(f"Reading {port} as {frame}")

        self.timer = self.create_timer(0.02, self.loop)

    def loop(self):
        for sensor in self.sensors:
            line = sensor["ser"].readline().decode(errors="ignore").strip()
            if not line:
                continue

            m = self.regex.search(line)
            if not m:
                continue

            us_cm = float(m.group(1))
            us_ok = int(m.group(2))
            tof_cm = float(m.group(3))
            tof_ok = int(m.group(4))

            now = self.get_clock().now().to_msg()
            frame = sensor["frame"]

            if tof_ok:
                msg = Range()
                msg.header.stamp = now
                msg.header.frame_id = frame
                msg.radiation_type = Range.INFRARED
                msg.field_of_view = 0.5
                msg.min_range = 0.02
                msg.max_range = 30.0
                msg.range = tof_cm / 100.0
                sensor["pub_tof"].publish(msg)

            if us_ok:
                msg = Range()
                msg.header.stamp = now
                msg.header.frame_id = frame
                msg.radiation_type = Range.ULTRASOUND
                msg.field_of_view = 0.5
                msg.min_range = 0.02
                msg.max_range = 6.0
                msg.range = us_cm / 100.0
                sensor["pub_us"].publish(msg)

def main():
    rclpy.init()
    node = ToFBridge()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == "__main__":
    main()
