#!/usr/bin/env python3
import serial
import pynmea2
import rclpy
from rclpy.node import Node
from sensor_msgs.msg import NavSatFix, NavSatStatus


class RdkGpsNode(Node):
    def __init__(self):
        super().__init__("rdk_gps_node")

        self.pub = self.create_publisher(NavSatFix, "/gps/fix", 10)

        self.port = "/dev/ttyS1"
        self.baud = 115200

        self.ser = serial.Serial(self.port, self.baud, timeout=1)

        self.get_logger().info(f"Reading GPS NMEA from {self.port} at {self.baud}")

        self.timer = self.create_timer(0.01, self.read_serial)

    def read_serial(self):
        try:
            line = self.ser.readline().decode("ascii", errors="ignore").strip()
            if not line.startswith("$"):
                return

            msg = pynmea2.parse(line)

            if msg.sentence_type != "GGA":
                return

            if not msg.latitude or not msg.longitude:
                return

            fix = NavSatFix()
            fix.header.stamp = self.get_clock().now().to_msg()
            fix.header.frame_id = "gps_link"

            fix.latitude = float(msg.latitude)
            fix.longitude = float(msg.longitude)

            try:
                fix.altitude = float(msg.altitude)
            except Exception:
                fix.altitude = 0.0

            fix.status.status = NavSatStatus.STATUS_FIX
            fix.status.service = (
                NavSatStatus.SERVICE_GPS |
                NavSatStatus.SERVICE_GLONASS |
                NavSatStatus.SERVICE_GALILEO |
                NavSatStatus.SERVICE_COMPASS
            )

            # Basic covariance from HDOP, rough placeholder.
            try:
                hdop = float(msg.horizontal_dil)
                variance = hdop * hdop
            except Exception:
                variance = 9.0

            fix.position_covariance = [
                variance, 0.0, 0.0,
                0.0, variance, 0.0,
                0.0, 0.0, variance * 2.0,
            ]
            fix.position_covariance_type = NavSatFix.COVARIANCE_TYPE_APPROXIMATED

            self.pub.publish(fix)

        except pynmea2.ParseError:
            return
        except Exception as e:
            self.get_logger().warn(f"GPS read/parse error: {e}")


def main():
    rclpy.init()
    node = RdkGpsNode()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
