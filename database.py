import os
import sqlite3

DB_PATH = "/home/pi/iot/smart_lighting.db"
DB_DIR = os.path.dirname(DB_PATH)

if not os.path.exists(DB_DIR):
    os.makedirs(DB_DIR)
    print(f"Directory {DB_DIR} was created.")

conn = sqlite3.connect(DB_PATH)
cursor = conn.cursor()

cursor.execute(
    """
    CREATE TABLE IF NOT EXISTS light_status(
        device_id TEXT NOT NULL,
        light_status INTEGER NOT NULL CHECK (light_status IN (0,1)),
        protocol TEXT NOT NULL,
        timestamp TEXT NOT NULL DEFAULT (datetime('now'))           
    )
"""
)
conn.commit()

cursor.execute(
    """
    CREATE TABLE IF NOT EXISTS metrics_latency(
        device_id1 TEXT NOT NULL,
        device_id2 TEXT NOT NULL,
        latency INTEGER NOT NULL,
        protocol TEXT NOT NULL,
        timestamp TEXT NOT NULL DEFAULT (datetime('now'))           
    )
"""
)
conn.commit()


def add_light_status(device_id, status, protocol, timestamp=None):
    status_int = 1 if status else 0  # Convert boolean to 1/0

    # If timestamp is passed, use it; otherwise, let the database handle the default timestamp
    if timestamp:
        cursor.execute(
            """
            INSERT INTO light_status (device_id, light_status, protocol, timestamp)
            VALUES (?, ?, ?, ?)
            """,
            (device_id, status_int, protocol, timestamp),
        )
    else:
        cursor.execute(
            """
            INSERT INTO light_status (device_id, light_status, protocol)
            VALUES (?, ?, ?)
            """,
            (device_id, status_int, protocol),
        )

    conn.commit()
    # print(
    #     f"Light status for {device_id} recorded: {status_int} via {protocol} at {timestamp if timestamp else 'default timestamp'}"
    # )


def add_metrics_latency(device_id1, device_id2, latency, protocol, timestamp=None):
    if timestamp:
        cursor.execute(
            """
            INSERT INTO metrics_latency (device_id1, device_id2, latency, protocol, timestamp)
            VALUES (?, ?, ?, ?, ?)
        """,
            (device_id1, device_id2, latency, protocol, timestamp),
        )
    else:
        cursor.execute(
            """
            INSERT INTO metrics_latency (device_id1, device_id2, latency, protocol)
            VALUES (?, ?, ?, ?)
        """,
            (device_id1, device_id2, latency, protocol),
        )

    conn.commit()
