#!/usr/bin/env python3
"""Quantitative diagnostics for an OrinBot ROS 2 SQLite bag.

Requires numpy, scipy, and rosbags. This measures topic timing, the recorded
TF path, and cross-cloud nearest-neighbor residuals. The residual is a scene
alignment diagnostic, not a calibrated extrinsic offset or object tracker.
"""

import argparse
import sqlite3
from collections import Counter, defaultdict, deque
from pathlib import Path

import numpy as np
from rosbags.typesys import Stores, get_typestore
from scipy.spatial import cKDTree
from scipy.spatial.transform import Rotation

LIDAR = "/unilidar/cloud"
OAK = "/oak/rgbd/points"
MARKERS = "/spatial_bb"


def stamp(msg):
    t = msg.header.stamp
    return t.sec + t.nanosec / 1e9


def transform_matrix(t):
    q = t.transform.rotation
    p = t.transform.translation
    result = np.eye(4)
    result[:3, :3] = Rotation.from_quat([q.x, q.y, q.z, q.w]).as_matrix()
    result[:3, 3] = [p.x, p.y, p.z]
    return result


def transforms_to_base(tf_messages):
    graph = {}
    for message in tf_messages:
        for t in message.transforms:
            parent, child = t.header.frame_id, t.child_frame_id
            matrix = transform_matrix(t)
            graph.setdefault(parent, []).append((child, matrix))
            graph.setdefault(child, []).append((parent, np.linalg.inv(matrix)))
    queue = deque([("base_link", np.eye(4))])
    found = {}
    while queue:
        frame, matrix = queue.popleft()
        if frame in found:
            continue
        found[frame] = matrix
        for child, edge in graph.get(frame, []):
            if child not in found:
                queue.append((child, matrix @ edge))
    return found


def points(msg, matrix, limit):
    if msg.row_step != msg.width * msg.point_step:
        raise ValueError("Padded PointCloud2 rows are not supported by this diagnostic")
    fields = {field.name: field for field in msg.fields}
    if any(name not in fields or fields[name].datatype != 7 for name in "xyz"):
        raise ValueError("Expected float32 x/y/z PointCloud2 fields")
    endian = ">" if msg.is_bigendian else "<"
    dtype = np.dtype({"names": ["x", "y", "z"],
                      "formats": [endian + "f4"] * 3,
                      "offsets": [fields[name].offset for name in "xyz"],
                      "itemsize": msg.point_step})
    raw = np.ndarray(msg.width * msg.height, dtype=dtype, buffer=msg.data)
    xyz = np.column_stack([raw[name] for name in "xyz"])
    xyz = xyz[np.isfinite(xyz).all(axis=1)]
    xyz = xyz[(np.linalg.norm(xyz, axis=1) > 0.2) & (np.linalg.norm(xyz, axis=1) < 8)]
    if len(xyz) > limit:
        xyz = xyz[np.linspace(0, len(xyz) - 1, limit, dtype=int)]
    return xyz @ matrix[:3, :3].T + matrix[:3, 3]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("database", type=Path, help="Downloaded rosbag .db3 file")
    parser.add_argument("--pairs", type=int, default=3)
    parser.add_argument("--max-time-difference", type=float, default=0.10)
    args = parser.parse_args()
    database = sqlite3.connect(f"file:{args.database}?mode=ro", uri=True)
    topics = {name: (id_, type_) for id_, name, type_ in database.execute(
        "SELECT id, name, type FROM topics")}
    store = get_typestore(Stores.ROS2_HUMBLE)

    def messages(topic, limit=None):
        if topic not in topics:
            return []
        id_, type_ = topics[topic]
        sql = "SELECT timestamp,data FROM messages WHERE topic_id=? ORDER BY timestamp"
        if limit is not None:
            sql += f" LIMIT {int(limit)}"
        return [(record / 1e9, store.deserialize_cdr(data, type_))
                for record, data in database.execute(sql, (id_,))]

    tf = [msg for topic in ("/tf_static", "/tf") for _, msg in messages(topic, 30)]
    frame_to_base = transforms_to_base(tf)
    lidars, oaks = messages(LIDAR), messages(OAK)
    print(f"LiDAR clouds: {len(lidars)}; OAK clouds: {len(oaks)}")
    for topic, data in ((LIDAR, lidars), (OAK, oaks)):
        if data:
            record_delay = np.array([record - stamp(msg) for record, msg in data])
            print(f"{topic}: frame={data[0][1].header.frame_id}; "
                  f"record-minus-header median={np.median(record_delay):.3f}s, "
                  f"p90={np.percentile(record_delay, 90):.3f}s")
    for topic, data in ((LIDAR, lidars), (OAK, oaks)):
        if data:
            frame = data[0][1].header.frame_id
            if frame in frame_to_base:
                print(f"TF base_link <- {frame}: translation_m="
                      f"{np.round(frame_to_base[frame][:3, 3], 4).tolist()}")
            else:
                print(f"No TF path from base_link to {frame}")
    if lidars and oaks and all(data[0][1].header.frame_id in frame_to_base
                               for data in (lidars, oaks)):
        picks = np.linspace(0, len(oaks) - 1, min(args.pairs, len(oaks)), dtype=int)
        for index in picks:
            oak_record, oak = oaks[index]
            _, lidar = min(lidars, key=lambda item: abs(stamp(item[1]) - stamp(oak)))
            delta = abs(stamp(lidar) - stamp(oak))
            if delta > args.max_time_difference:
                print(f"Cloud pair skipped: header gap {delta:.3f}s")
                continue
            lidar_xyz = points(lidar, frame_to_base[lidar.header.frame_id], 10000)
            oak_xyz = points(oak, frame_to_base[oak.header.frame_id], 6000)
            if not len(lidar_xyz) or not len(oak_xyz):
                print("Cloud pair skipped: no finite points in range")
                continue
            distances, nearest = cKDTree(lidar_xyz).query(oak_xyz)
            close = distances < 0.5
            print(f"Cloud pair at {stamp(oak):.3f}: header_gap={delta:.3f}s; "
                  f"points={len(lidar_xyz)}/{len(oak_xyz)}; "
                  f"NN median={np.median(distances):.3f}m; "
                  f"p90={np.percentile(distances, 90):.3f}m; "
                  f"within_0.25m={np.mean(distances < 0.25):.1%}; "
                  f"within_0.50m={np.mean(close):.1%}")
            if close.any():
                residual = lidar_xyz[nearest[close]] - oak_xyz[close]
                print(f"  Close-point median residual xyz_m="
                      f"{np.round(np.median(residual, axis=0), 3).tolist()}")
    if MARKERS in topics:
        counts, labels = Counter(), Counter()
        ranges = defaultdict(list)
        for _, msg in messages(MARKERS):
            counts[len(msg.markers)] += 1
            for marker in msg.markers:
                if marker.text:
                    labels[marker.text] += 1
                    position = marker.pose.position
                    ranges[marker.text].append(np.linalg.norm(
                        [position.x, position.y, position.z]))
        print(f"Marker count per message: {dict(counts)}; labels: {dict(labels)}")
        for label, values in sorted(ranges.items()):
            print(f"  {label}: marker-position range min/median/max="
                  f"{min(values):.3f}/{np.median(values):.3f}/{max(values):.3f}m")
        print("Marker IDs are visualization IDs; this does not establish object tracking.")
    database.close()


if __name__ == "__main__":
    main()
