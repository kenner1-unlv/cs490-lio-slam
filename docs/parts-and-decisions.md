# Construction, parts and budget decisions

This inventory combines the completed-project reports, the hardware named in this repository, and the team's firsthand construction account. The two-person CS/engineering team selected and ordered parts, planned the cargo and battery layout, fabricated and fiber-laser welded the chassis, built the component tray and exterior panels, made and routed the power/CAN/PoE/network harnesses, and integrated the electronics and software. The engineering teammate led power calculations and wiring/fuse plans; the CS teammate led programming, model selection and integration with selected hardware. The final report's [build and system figures](build-and-system-figures.md) are included here, including the integrated power/network/CAN layout. This is a design record, **not** a priced bill of materials. No receipts, item quantities for every accessory, or total cost were available in the reviewed sources. Add them before publishing a numeric budget claim.

| Part or subsystem | Reason for selection | Tradeoff or result | Cost evidence needed |
| --- | --- | --- | --- |
| Fabricated chassis, component tray and exterior panels | Carry and protect the selected drive, compute, power and sensor layout | Team-built physical integration; dimensions, materials and measured fit have not yet been documented in this repo | Material invoices, fabrication consumables and drawings |
| Four 500 W, 48 V-class hub motors and swing-arm hardware | Strong, low-cost, durable scooter wheels with payload capacity | The robot moved and turned, but four fixed contact patches caused excessive scrub and poor in-place turning; controlled autonomous driving was not reached | Supplier, purchase price, quantity, reused/donated status |
| Four Flipsky 75100 V2 Pro VESCs | Configurable motor control and CAN telemetry | ERPM, duty and current are observable in ROS 2 bags, but wheel-ground motion and slip remain unmeasured | Invoices and firmware/settings record |
| 16S LiFePO4 drive battery and separate compute/sensor supply | Keep high-current drive loads away from sensitive computers and sensors | The architecture is documented; endurance, sag and thermal margin are not measured here | Battery specifications, cost, fuse/regulator drawings and runtime test |
| Jetson Orin | Central ROS 2 integration, recording and experimental odometry | Distributed devices reduce its direct hardware IO, while adding network and clock complexity | Exact model, acquisition status and price |
| Unitree L2 | 3D geometry and IMU for odometry and mapping experiments | Point cloud and IMU recorded; calibration and odometry accuracy remain open | Price and mount dimensions |
| OAK 4-D Pro | On-device RGB-D and spatial inference | RGB, depth cloud and spatial markers recorded; person tracking and range accuracy remain unverified | Price, calibration data and mount measurements |
| RDK X5 with front shoulder camera, ToF and ultrasonic modules | Local image/range sensing and angled near-field coverage | ToF/ultrasonic topics recorded; shoulder image recording, coverage and obstacle stopping not validated by the reviewed bags | Sensor and board prices, printed-part materials |
| Proposed NOVA20 line-array module(s) | Compare 20 channelized near-field distances with existing single-point returns, possibly in horizontal/vertical crossed orientations | [Comparison proposal](near-field-sensor-upgrade.md) only; no purchase, integration or drop-off result claimed | One-module sample/price, then four-module, connector, power and enclosure costs if the cross proves useful |
| ESP32 CAN bridge, PoE/network hardware, printed housings | Modular communication and serviceable physical layout | Bridge and startup code preserved; network latency, power reliability and fit lack measured results | Board, switch, cable, filament and fabrication costs |

## The steering decision

The fixed-wheel layout was practical for getting a moving platform within a semester. It proved drive torque, motor command routing and telemetry. The robot could turn, but tire scrub prevented the desired in-place rotation and undermined repeatable control for an autonomous route. This is a firsthand project outcome; the current bags do not contain surveyed turn paths or a controlled skid-steer benchmark. The next build should measure the baseline and compare two-wheel drive with casters, Ackermann steering and independently steered hub-motor pods before claiming better maneuverability.

## Materials still to locate

- Original itemized purchases and dates, plus which parts were already owned or contributed.
- Test-bench equipment and fixture costs for repeatable steering, power and sensor calibration trials.
- Dimensioned chassis, component-tray and exterior-panel drawings; material and fabrication notes; measured mass, wheelbase, track width, wheel diameter and ground clearance. The report photo is a build-stage view, not a dimensioned drawing.
- Connector-level electrical schematics, fuse and regulator ratings, plus measured current and battery runtime. The report's integrated wiring layout is included, but it does not supply all these values.
- Additional photos showing assembly stages, internal mounts and cable routing, with author/photographer credit. The report already supplies a physical-platform and installed shoulder-module photo.
- Individual contribution record for frame, electrical, firmware, ROS 2, perception and testing work.

The May 2026 final OrinBot report describes the assembled chassis, 3D-printed housings, separated power domains and proposed servo-steered wheel pods. The earlier CS 490 planning report describes the initial Ethernet-first, LiDAR-first and CAN-control choices. The February perception-stack report explicitly marks sensor offsets as provisional at that time. Use these dated records to explain how the design changed; do not treat the proposed servo-steered pod as built hardware.
