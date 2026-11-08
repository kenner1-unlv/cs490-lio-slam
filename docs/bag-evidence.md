# Recorded evidence and its limits

These are the five May 7, 2026 ROS 2 bags in `rosbags/`. Counts below come from each checked-in `metadata.yaml`; only the short 12:49:55 database has been decoded for the perception measurements in this study. A topic count proves that messages were recorded under a topic name. It does not establish valid contents, clock synchronization, physical movement, or tracking quality.

| Bag folder suffix | Duration (s) | Total messages | Topics | `/unilidar/cloud` | `/oak/rgbd/points` | `/odom_corrected` | OAK detections |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| `integrated_20260507_124955` | 9.5 | 16,163 | 30 | 113 | 36 | 3,199 | 146 |
| `1min_20260507_130713` | 59.4 | 120,866 | 27 | 0 | 473 | 1 | 1,747 |
| `1min_cleancloud_20260507_131959` | 59.4 | 412,243 | 27 | 0 | 404 | 201,371 | 1,766 |
| `1min_rawcloud_20260507_132611` | 59.4 | 98,773 | 24 | 673 | 0 | 12,771 | 0 |
| `1min_rawcloud_20260507_132816` | 59.1 | 101,700 | 28 | 698 | 460 | 144 | 1,711 |

The recording profiles differ. Two minute-long bags have no `/unilidar/cloud`; one raw-cloud bag lacks the OAK cloud and detection topics. `/odom_corrected` ranges from one to more than 200,000 messages per minute, far too much variation to describe as a stable validated odometry rate. The large rate may reflect an excessive publisher loop, repeated data, or a recording configuration problem. Message contents and timestamps must be examined before deciding why.

The short bag records `/joy`, all four VESC motor ERPM/current/duty channels, LiDAR and OAK streams, shoulder ranges, TF, robot description, and odometry during the same 9.5-second window. It is the best existing capture for a first cross-sensor check. It does not include a planner command or documented ground-truth path. The motor command bridge in this repository consumes joystick axes directly, so the bag cannot substantiate autonomous driving.

## Reproduction

Run `python tools/bag_summary.py rosbags/<bag-folder>` after downloading metadata through Git LFS. If the database is also present, the script prints each topic's first and last recording timestamps and largest gap. The `.db3` files are large; the short bag is about 269 MB and the minute-long ones can be around 2 GB. Keep the raw bags intact and publish derived metrics beside them.

The [perception evaluation](perception-evaluation.md) records what was decoded from the short database. Future evaluations should keep the bag name, script version, sensor frames, time pairing rule, point filtering, test scene, and measurement uncertainty with every result.
