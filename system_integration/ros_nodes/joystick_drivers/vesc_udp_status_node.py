import re
import socket

import rclpy
from rclpy.node import Node

from std_msgs.msg import String, Float32


V1_RE = re.compile(
    r"V1\s+id=(?P<id>\d+)\s+erpm=(?P<erpm>-?\d+)\s+cur=(?P<cur>-?\d+(?:\.\d+)?)\s+duty=(?P<duty>-?\d+(?:\.\d+)?)"
)


class VescUdpStatusNode(Node):
    def __init__(self):
        super().__init__('vesc_udp_status_node')

        self.listen_ip = "0.0.0.0"
        self.listen_port = 20000

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
        self.sock.bind((self.listen_ip, self.listen_port))
        self.sock.setblocking(False)

        self.pub_raw = self.create_publisher(String, '/vesc/debug_raw', 10)

        self.motor_pubs = {}

        self.timer = self.create_timer(0.005, self.timer_callback)

        self.get_logger().info(
            f"Listening for ESP32/VESC telemetry on UDP {self.listen_ip}:{self.listen_port}"
        )

    def get_motor_pubs(self, motor_id: int):
        if motor_id not in self.motor_pubs:
            prefix = f'/vesc/motor_{motor_id}'

            self.motor_pubs[motor_id] = {
                'erpm': self.create_publisher(Float32, f'{prefix}/erpm', 10),
                'current': self.create_publisher(Float32, f'{prefix}/current', 10),
                'duty': self.create_publisher(Float32, f'{prefix}/duty', 10),
            }

            self.get_logger().info(f"Created ROS topics for motor {motor_id}")

        return self.motor_pubs[motor_id]

    def publish_float(self, pub, value: float):
        msg = Float32()
        msg.data = float(value)
        pub.publish(msg)

    def handle_line(self, text: str):
        raw_msg = String()
        raw_msg.data = text
        self.pub_raw.publish(raw_msg)

        match = V1_RE.search(text)
        if not match:
            return

        motor_id = int(match.group('id'))
        erpm = float(match.group('erpm'))
        current = float(match.group('cur'))
        duty = float(match.group('duty'))

        pubs = self.get_motor_pubs(motor_id)

        self.publish_float(pubs['erpm'], erpm)
        self.publish_float(pubs['current'], current)
        self.publish_float(pubs['duty'], duty)

    def timer_callback(self):
        while True:
            try:
                data, addr = self.sock.recvfrom(2048)
            except BlockingIOError:
                return
            except Exception as e:
                self.get_logger().error(f"UDP receive error: {e}")
                return

            text = data.decode('utf-8', errors='replace').strip()

            # tcpdump shows clean payloads, but this protects against any packed/messy lines.
            for line in text.splitlines():
                line = line.strip()
                if line:
                    self.handle_line(line)

    def destroy_node(self):
        try:
            self.sock.close()
        finally:
            super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = VescUdpStatusNode()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        try:
            rclpy.shutdown()
        except Exception:
            pass


if __name__ == '__main__':
    main()
