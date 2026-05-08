#!/usr/bin/env python3
import socket
import time

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import Joy


class JoyToVescUdp(Node):
    def __init__(self):
        super().__init__('joy_to_vesc_udp')

        # Edge bridge target
        self.EDGE_IP = "192.168.50.68"
        self.EDGE_PORT = 20001

        # Tank steering motor groups
        self.left_motors = [12, 22]
        self.right_motors = [11, 21]

        # Tuning
        self.max_rpm = 3500
        self.deadzone = 0.16
        self.send_hz = 50.0              # keepalive resend rate
        self.command_timeout = 0.50      # if no joy msg for this long, command 0
        self.ramp_step = 250             # rpm per timer tick for softer accel/decel

        # DualSense stick axes
        self.left_axis_index = 1         # left stick vertical
        self.right_axis_index = 5        # right stick vertical on many Linux mappings
        # If your right stick vertical is actually some other index, change only this.

        # Buttons
        self.square_button_index = 2     # square
        self.l1_button_index = 4
        self.r1_button_index = 5

        self.sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

        self.target_left_rpm = 0
        self.target_right_rpm = 0

        self.sent_left_rpm = 0
        self.sent_right_rpm = 0

        self.last_joy_time = time.time()
        self.last_bumper_time = 0.0

        self.subscription = self.create_subscription(
            Joy, '/joy', self.joy_callback, 10
        )

        self.timer = self.create_timer(1.0 / self.send_hz, self.timer_callback)

        self.get_logger().info(
            f"UDP -> Edge101 | tank drive | max_rpm={self.max_rpm} | "
            f"left_motors={self.left_motors} | right_motors={self.right_motors}"
        )

    def send_rpm(self, motor_id: int, rpm: int):
        msg = f'{{"t":"cmd","id":{motor_id},"mode":"rpm","value":{rpm}}}'
        try:
            self.sock.sendto(msg.encode('utf-8'), (self.EDGE_IP, self.EDGE_PORT))
        except Exception as e:
            self.get_logger().error(f"UDP send fail for motor {motor_id}: {e}")

    def send_group(self, motor_ids, rpm: int):
        for motor_id in motor_ids:
            self.send_rpm(motor_id, rpm)

    def kill_all_motors(self):
        for motor_id in self.left_motors + self.right_motors:
            self.send_rpm(motor_id, 0)
        self.target_left_rpm = 0
        self.target_right_rpm = 0
        self.sent_left_rpm = 0
        self.sent_right_rpm = 0
        self.get_logger().warn("Square pressed: all motors set to 0 RPM")

    def apply_deadzone(self, val: float) -> int:
        if abs(val) < self.deadzone:
            return 0

        scaled = (abs(val) - self.deadzone) / (1.0 - self.deadzone)
        sign = 1 if val > 0 else -1
        return int(sign * scaled * self.max_rpm)

    def get_axis(self, msg: Joy, index: int) -> float:
        if 0 <= index < len(msg.axes):
            return msg.axes[index]
        return 0.0

    def button_pressed(self, msg: Joy, index: int) -> bool:
        return 0 <= index < len(msg.buttons) and bool(msg.buttons[index])

    def joy_callback(self, msg: Joy):
        now = time.time()
        self.last_joy_time = now

        # Tank steering:
        # up on stick should be positive forward command
        left_raw = self.get_axis(msg, self.left_axis_index)
        right_raw = self.get_axis(msg, self.right_axis_index)

        # Emergency stop
        if self.button_pressed(msg, self.square_button_index):
            self.kill_all_motors()
            return

        # Adjust max RPM with bumpers
        if self.button_pressed(msg, self.r1_button_index) and (now - self.last_bumper_time > 0.3):
            self.max_rpm = min(8000, self.max_rpm + 500)
            self.last_bumper_time = now
            self.get_logger().info(f"Max RPM increased to {self.max_rpm}")

        if self.button_pressed(msg, self.l1_button_index) and (now - self.last_bumper_time > 0.3):
            self.max_rpm = max(1000, self.max_rpm - 500)
            self.last_bumper_time = now
            self.get_logger().info(f"Max RPM decreased to {self.max_rpm}")

        self.target_left_rpm = self.apply_deadzone(left_raw)
        self.target_right_rpm = self.apply_deadzone(right_raw)

    def step_toward(self, current: int, target: int, step: int) -> int:
        if current < target:
            return min(current + step, target)
        if current > target:
            return max(current - step, target)
        return current

    def timer_callback(self):
        now = time.time()

        # Safety timeout: if joystick messages stop arriving, command zero
        if (now - self.last_joy_time) > self.command_timeout:
            self.target_left_rpm = 0
            self.target_right_rpm = 0

        # Soft ramp instead of abrupt jumps
        new_left = self.step_toward(self.sent_left_rpm, self.target_left_rpm, self.ramp_step)
        new_right = self.step_toward(self.sent_right_rpm, self.target_right_rpm, self.ramp_step)

        # Always resend at fixed rate so held sticks keep commanding speed
        self.send_group(self.left_motors, new_left)
        self.send_group(self.right_motors, new_right)

        self.sent_left_rpm = new_left
        self.sent_right_rpm = new_right

    def destroy_node(self):
        try:
            self.sock.close()
        finally:
            super().destroy_node()


def main(args=None):
    rclpy.init(args=args)
    node = JoyToVescUdp()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == '__main__':
    main()
