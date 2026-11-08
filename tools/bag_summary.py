#!/usr/bin/env python3
"""Summarize ROS 2 SQLite bags using PyYAML and the Python standard library.

Metadata gives topic counts. With a downloaded .db3 file, this also reports
recording-time spans and gaps; these are not sensor header timestamps.
"""

import argparse
import sqlite3
from pathlib import Path

import yaml


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("bag", type=Path, help="ROS 2 bag directory")
    args = parser.parse_args()
    info = yaml.safe_load((args.bag / "metadata.yaml").read_text())
    if not isinstance(info, dict):
        parser.error("metadata.yaml is a Git LFS pointer; run git lfs pull for metadata")
    info = info[
        "rosbag2_bagfile_information"
    ]
    duration = info["duration"]["nanoseconds"] / 1e9
    print(f"Bag: {args.bag.name}")
    print(f"Duration: {duration:.3f} s; messages: {info['message_count']}")
    files = list(args.bag.glob("*.db3"))
    connection = None
    if len(files) == 1 and files[0].stat().st_size > 1000:
        connection = sqlite3.connect(f"file:{files[0]}?mode=ro", uri=True)
    elif files:
        print("Database is a Git LFS pointer; download it for timing analysis.")
    print("topic,count,mean_hz,first_record_s,last_record_s,max_record_gap_s")
    for item in sorted(info["topics_with_message_count"], key=lambda x: x["topic_metadata"]["name"]):
        name = item["topic_metadata"]["name"]
        count = item["message_count"]
        timing = ["", "", ""]
        if connection is not None:
            row = connection.execute("SELECT id FROM topics WHERE name=?", (name,)).fetchone()
            if row:
                stamps = [r[0] for r in connection.execute(
                    "SELECT timestamp FROM messages WHERE topic_id=? ORDER BY timestamp", (row[0],)
                )]
                if stamps:
                    timing = [f"{stamps[0]/1e9:.6f}", f"{stamps[-1]/1e9:.6f}",
                              f"{max((b-a for a,b in zip(stamps,stamps[1:])), default=0)/1e9:.6f}"]
        print(f"{name},{count},{count/duration:.2f},{','.join(timing)}")
    if connection is not None:
        connection.close()


if __name__ == "__main__":
    main()
