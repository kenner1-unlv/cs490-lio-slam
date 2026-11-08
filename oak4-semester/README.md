# OAK 4 D Pro semester integration

The OAK 4 D Pro was the robot's forward perception computer, not just a camera attached to the Orin. Its RGB camera, stereo pair and IMU could produce image, depth and inertial streams while a spatial neural-network path ran on the device. Luxonis documents the [OAK 4 D Pro hardware](https://docs.luxonis.com/hardware/products/OAK%204%20D%20Pro) and [RVC4 platform](https://docs.luxonis.com/hardware/platform/rvc/rvc4). The team integrated those outputs with the Unitree L2, robot frames and motor telemetry in ROS 2, then recorded and visualized the combined system. This is the semester result; it does not imply a calibrated fused obstacle model.

## Preserved May app configuration

| File | Origin and role |
| --- | --- |
| [oakapp.toml](oakapp.toml) | Exact manifest from the Orin's `/data/luxonis/apps/ros/ros-driver-spatial-bb/`, SHA-256 `51f54c5af7804b3758c68ed58710df0b45b345cf2a1f4f377fb8b86e5a12062d`. It packages ROS 2 Kilted with `ros-kilted-depthai-ros`, forces Fast DDS, and launches `depthai_filters spatial_bb.launch.py`. |
| [spatial_bb.yaml](spatial_bb.yaml) | Configuration extracted from the archived app's installed `depthai_filters` package. The package's 239-byte file has SHA-256 `23735ac4ed5fe8372a2c89d9151da90a6abf9b934338dccd715a60586d145e59`; the checked-in 238-byte copy has only one extra end-of-file blank line removed and SHA-256 `334e4c3443271586a1e1b1b813a82963a15e773ad1ce7c270d62215ac258448c`. It enables a spatial NN path and RGB-D, stereo subpixel processing, and RGB/depth passthrough for the filter. This is vendor ROS configuration, not an original model definition; see the [Luxonis MIT license](LICENSE-Luxonis.txt). |

The archived package on the Orin is `example-apps-ros-driver-spatial-bb_0-9-1.oakapp`, identifier `example.apps.ros.driver-spatial-bb`, version `0.9.1`, built May 3–4, 2026. Its SHA-256 is `a870802ac125e73663520f313f18b0330ec46833366aa29bf2031f274d76d9b4`. The roughly 943 MB package is not copied into Git. Its embedded metadata confirms the launch command above. The team-modified manifest differs from the upstream [Luxonis OAK ROS example](https://github.com/luxonis/oak-examples/tree/main/apps/ros/ros-driver-spatial-bb) mainly in app version and Fast DDS startup. The ROS driver, filter and packaged YAML are Luxonis components; the team's contribution was selection, packaging, middleware/network integration, robot TF, visualization, recording and debugging across the Orin and OAK.

The folder next to this app on the Orin also has a `parameters.yaml` for a **different custom-workspace example**. It is not the configuration loaded by `spatial_bb.launch.py`. The latter defaults to the packaged `spatial_bb.yaml`, as confirmed by the installed launch file extracted from the archived app. The YAML sets `i_nn_type: spatial` but does not name a particular neural model, so the May bags should not be labeled as a specific YOLO model without a separate runtime record.

## Reconstructed data path

```mermaid
flowchart LR
    RGB[OAK RGB sensor] --> DRIVER[OAK ROS driver on ROS 2 Kilted]
    STEREO[OAK stereo pair] --> DRIVER
    IMU[OAK IMU] --> DRIVER
    YAML[spatial_bb.yaml] --> DRIVER
    DRIVER --> CLOUD[/oak/rgbd/points]
    DRIVER --> DET[/oak/nn/spatial_detections]
    DRIVER --> IMAGE[/oak/rgb/image_raw and OAK IMU]
    DET --> FILTER[SpatialBB ROS filter] --> MARKERS[/spatial_bb markers]
    CLOUD --> ORIN[Orin ROS 2 Humble over wired network]
    IMAGE --> ORIN
    DET --> ORIN
    MARKERS --> ORIN
    L2[Unitree L2 cloud and IMU] --> ORIN
    ORIN --> TF[Robot frames and RViz]
    ORIN --> BAG[Integrated rosbag]
```

The installed Luxonis `spatial_bb.launch.py` includes the ROS driver's launch and composes the `depthai_filters::SpatialBB` node with detection, image and camera-info remappings. The archived app and its README name `/oak/rgbd/points` and `/spatial_bb` as outputs. The [short May 7 bag](../docs/bag-evidence.md) contains 36 OAK RGB-D clouds, 130 RGB images, 1,368 OAK IMU messages, 146 spatial-detection messages and 108 marker messages in 9.46 seconds, alongside 113 Unitree L2 clouds and other robot streams. That is direct evidence that the two perception sources were present in one recorded ROS 2 environment.

The package build date, launch configuration and recorded topic names are consistent with this being the app used for the May recording. We did not find a retained May 7 `oakctl app list` or launch log that proves the exact package ID active at the instant of capture. The [point-cloud comparison](../docs/perception-evaluation.md) shows variable overlap after applying the recorded TF; it is not a sensor calibration or proof of robustness while driving.

## What came later

The separate [OAK 4 CV Playground](https://github.com/kenner1-unlv/Oak-4-CV-playground) later expanded on-device segmentation, stereo range, provisional object identities and a Unitree L2 preview relay. That relay serves sampled, uncalibrated LiDAR points for a browser overlay. It is a continuation of the robot's OAK/L2 learning, not a capability attributed to the CS 490 semester app or May bags. Its later RVC4 model-selection YAML files belong to that repository.
