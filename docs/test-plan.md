# Next evidence tests

Keep raw bags, scripts, configuration, photos/video, measured reference values and a short result sheet for each run. Before testing, record the hardware revision, wheel/tire condition, battery state, surface, sensor mounts, software commit, date and operator. Report failures and missing topics alongside successes.

| Question | Test and recorded evidence | Result to publish |
| --- | --- | --- |
| How poor was fixed-wheel turning? | Mark a safe flat course; repeat straight runs, 90-degree turns and turn-in-place attempts under manual control. Record commanded motor RPM, four VESC ERPMs, LiDAR/IMU yaw, video, surface and battery state. Use surveyed start/end pose or overhead tracking as ground truth. | Turn completion rate, heading error, turn radius, time, slip and spread across repeated trials. Show the failed runs. |
| Does a different steering design help? | Repeat the same course with the proposed steerable pod or other selected geometry, matching speed, surface and payload. Record steering angle as well as drive state. | Comparative turn and odometry metrics. A CAD rendering alone is not a test result. |
| Are LiDAR and RGB-D spatially aligned? | Use the stationary multi-position target procedure in [perception evaluation](perception-evaluation.md). Record physical measurements and raw sensor headers. | Fitted extrinsic correction, residuals and uncertainty per target position, plus a held-out check. |
| Is object distance useful? | Place known targets at measured ranges and angles; compare OAK spatial depth and LiDAR returns on the same object. | Signed and absolute range error by distance/angle, detection coverage and missing values. |
| Does object/person tracking work? | Record annotated stationary and moving object/person sequences. Preserve image frames, classes, confidences, 3D positions and persistent IDs if the tracker supplies them. | Detection recall, range error, continuity and identity switches. Label detection and tracking as separate capabilities. |
| Is Point-LIO usable for motion estimation? | Run repeated surveyed straight and turn paths after turning is controllable. Record `/odom_corrected`, raw input, TF and ground truth. Investigate the current 1-to-201,371-message-per-minute spread first. | Translation and heading error, drift per meter, output gaps/rate and failure conditions. |
| Can the integrated system run reliably? | Repeat cold starts and a timed loaded run. Record boot/service logs, topic uptime, packet gaps, CPU/GPU temperature and battery voltage/current. | Start success rate, uptime, worst gaps, power draw and observed faults. Do not claim shift-length endurance until measured. |

Autonomous waypoint and obstacle-avoidance tests belong after turning, command arbitration, stop behavior, calibrated frames and state estimation have passed. A joystick topic or motor telemetry alone does not verify planner-driven motion.
