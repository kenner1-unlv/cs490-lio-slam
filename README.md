# CS 490 OrinBot Evidence Repository

This repository contains supporting evidence for the CS 490 OrinBot independent study project.

The project goal was to build and integrate a ROS2-based mobile robotics platform around the NVIDIA Jetson Orin. The final system combines motor control, LiDAR perception, RGB-D perception, edge sensors, Point-LIO odometry, VESC telemetry, ROS2 visualization, and rosbag data recording.

## Repository Contents

- rosbags/  
  ROS2 bag recordings from integrated robot tests.

- system_integration/  
  Startup scripts, launch/service evidence, and integration files.

- .gitattributes  
  Git LFS tracking for large rosbag database files.

## What the ROS Bags Demonstrate

The included ROS2 bags document live integrated operation of the OrinBot stack.

The recorded data includes:

- joystick command input
- Unitree L2 LiDAR point cloud
- Unitree L2 IMU stream
- Point-LIO corrected odometry
- OAK 4-D RGB-D point cloud
- OAK 4-D IMU data
- OAK neural/spatial detections
- front-left and front-right ToF sensors
- front-left and front-right ultrasonic sensors
- robot TF and static TF transforms
- robot description / URDF data
- VESC telemetry from all four motor controllers
- VESC debug/status messages

## Key Topics

/joy
/unilidar/cloud
/unilidar/imu
/odom_corrected
/oak/rgbd/points
/oak/imu/data
/oak/nn/spatial_detections
/spatial_bb
/tof_front_left/tof
/tof_front_left/ultrasonic
/tof_front_right/tof
/tof_front_right/ultrasonic
/vesc/debug_raw
/vesc/motor_11/current
/vesc/motor_11/duty
/vesc/motor_11/erpm
/vesc/motor_12/current
/vesc/motor_12/duty
/vesc/motor_12/erpm
/vesc/motor_21/current
/vesc/motor_21/duty
/vesc/motor_21/erpm
/vesc/motor_22/current
/vesc/motor_22/duty
/vesc/motor_22/erpm
/tf
/tf_static
/robot_description
/joint_states

## Main System Result

The final integrated system includes:

- Jetson Orin main ROS2 computer
- Unitree L2 LiDAR and IMU
- OAK 4-D RGB-D perception camera
- Point-LIO corrected odometry
- RDK X5 / shoulder perception stack support
- left/right ToF and ultrasonic range sensors
- PS4 joystick command input
- UDP command bridge
- ESP32 CAN bridge
- 29-bit extended CAN frames
- four Flipsky VESC motor controllers
- four 500 W hub motors
- VESC telemetry returned to the Orin
- RViz visualization
- ROS2 bag recording

## Replay Instructions

On a ROS2 Humble machine with the project workspace sourced:

source /opt/ros/humble/setup.bash
source /data/ros_ws/install/setup.bash

cd /data/rosbags
ros2 bag play BAG_FOLDER_NAME --clock --loop

For RViz playback:

rviz2 --ros-args -p use_sim_time:=true

Recommended RViz fixed frame:

base_link

Useful RViz displays:

- TF
- PointCloud2: /unilidar/cloud
- PointCloud2: /oak/rgbd/points
- Odometry: /odom_corrected
- MarkerArray: /spatial_bb

## Notes

Some raw Unitree L2 point cloud inspection commands may print:

sequence size exceeds remaining buffer

This was observed when using some ROS2 CLI tools against the raw LiDAR cloud. The LiDAR stream was still usable in RViz and by the Point-LIO pipeline. For report evidence, the important artifacts are the recorded topic list, message counts, RViz screenshots, and successful /odom_corrected output.

## CS 490 Evidence Summary

This repository is not intended to be read line-by-line as source code. It is an evidence package showing that the robot produced synchronized ROS2 data during integrated testing.

The corresponding technical report explains the system architecture, hardware/software integration, testing process, and final results.
