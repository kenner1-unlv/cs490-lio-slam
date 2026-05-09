# Orinbot ESP32 CAN Bus Bridge

ESP32-based CAN bus bridge for Orinbot motor control.

This module receives movement commands from the Jetson Orin over UDP and translates them into CAN frames for the motor controllers. It is part of the Orinbot distributed embedded control architecture, where the Jetson Orin handles high-level ROS2 control and the ESP32 handles low-level CAN transmission.

## Current status

Working / tested features:

- ESP32 receives UDP command packets from the Orin
- ESP32 transmits CAN frames to the motor controller bus
- CAN bus runs at 500 kbps
- VESC controllers use 29-bit extended CAN frames
- Joystick / ROS2 command path can reach the motor controllers
- ESP32 keeps low-level motor bridge logic separate from the Orin ROS2 stack

## Hardware

### ESP32 CAN bridge

The bridge has been tested with ESP32-based CAN hardware, including:

- DFRobot Edge101 CAN board
- ESP32 CAN transceiver setups
- Earlier Waveshare CAN transceiver testing

Useful references:

- ESP32 TWAI / CAN controller documentation: https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/twai.html
- VESC CAN packet reference / community notes: https://github.com/vedderb/bldc
- Flipsky VESC product documentation: https://flipsky.net/pages/software-and-firmware

## CAN bus settings

Observed / intended CAN bus settings:

- CAN bitrate: 500000
- Frame type: extended 29-bit CAN
- Termination: approximately 60 ohms across CANH/CANL when both ends are terminated
- Motor controllers: Flipsky VESC-class controllers
- Typical status traffic: status frames around 20 Hz

Important detail:

The VESC command frames must use extended CAN identifiers. Standard 11-bit CAN IDs are not the correct command format for the VESC bus.

## Wiring strategy

Basic wiring:

- ESP32 CAN TX/RX pins connect to CAN transceiver
- CAN transceiver CANH connects to CAN bus CANH
- CAN transceiver CANL connects to CAN bus CANL
- ESP32 ground shares reference with motor controller / CAN transceiver ground
- CAN bus is terminated at both ends

General CAN bus path:

Jetson Orin -> UDP command packet -> ESP32 -> CAN transceiver -> CAN bus -> VESC motor controllers

## UDP strategy

The Orin sends command packets over UDP to the ESP32.

The ESP32 receives the UDP packet, decodes the requested drive command, and emits CAN frames to the VESC controllers.

This keeps the Orin focused on ROS2 command generation while the ESP32 handles deterministic low-level bus output.

## Robot control strategy

The control stack is intentionally modular:

ROS2 joystick / planner / autonomy node
-> cmd_vel-style command
-> Orin UDP bridge
-> ESP32 CAN bridge
-> VESC extended CAN command
-> hub motors

This means future autonomy code can reuse the same low-level motor pathway that was tested with joystick control.

## Debugging notes

Useful Linux CAN checks on the Orin or Linux CAN adapter:

- ip link set can0 up type can bitrate 500000
- candump can0
- ip -details -statistics link show can0

Useful symptoms:

- BUS-OFF usually means wiring, bitrate, grounding, or transceiver issue
- ERROR-PASSIVE usually means the bus is seeing errors but not fully dead
- Only seeing some controllers may indicate wiring, termination, or node power problems
- Seeing standard IDs when extended IDs are expected usually means the command encoding is wrong

## Planned next steps

- Preserve final known-good ESP32 CAN bridge firmware.
- Add a packet format description for Orin-to-ESP32 UDP commands.
- Add VESC CAN ID mapping for each wheel.
- Add safety timeout behavior if UDP commands stop arriving.
- Add heartbeat/status reporting from ESP32 back to Orin.
- Add ROS2 bridge documentation for the command path.
