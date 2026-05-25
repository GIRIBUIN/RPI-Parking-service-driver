import os
import sqlite3
from pathlib import Path


BASE_DIR = Path(__file__).resolve().parent
DB_NAME = os.environ.get("PARKING_DB_PATH", str(BASE_DIR / "parking.db"))
SCHEMA_PATH = BASE_DIR / "schema.sql"


def get_connection():
    conn = sqlite3.connect(DB_NAME)
    conn.row_factory = sqlite3.Row
    return conn


def init_db():
    conn = get_connection()

    with open(SCHEMA_PATH, "r", encoding="utf-8") as f:
        schema = f.read()

    conn.executescript(schema)
    conn.commit()
    conn.close()


def insert_parking_log(
    parking_state=None,
    risk_state=None,
    front_distance=None,
    rear_distance=None,
    side_distance=None
):
    conn = get_connection()

    conn.execute("""
        INSERT INTO parking_log (
            parking_state,
            risk_state,
            front_distance,
            rear_distance,
            side_distance
        )
        VALUES (?, ?, ?, ?, ?)
    """, (
        parking_state,
        risk_state,
        front_distance,
        rear_distance,
        side_distance
    ))

    conn.commit()
    conn.close()


def insert_gate_log(gate_state):
    conn = get_connection()

    conn.execute("""
        INSERT INTO gate_log (
            gate_state
        )
        VALUES (?)
    """, (gate_state,))

    conn.commit()
    conn.close()


def insert_event_log(topic, event_type, payload):
    conn = get_connection()

    conn.execute("""
        INSERT INTO event_log (
            topic,
            event_type,
            payload
        )
        VALUES (?, ?, ?)
    """, (
        topic,
        event_type,
        payload
    ))

    conn.commit()
    conn.close()


def get_latest_parking_state():
    conn = get_connection()

    cursor = conn.execute("""
        SELECT *
        FROM parking_log
        ORDER BY id DESC
        LIMIT 1
    """)

    row = cursor.fetchone()

    conn.close()

    return dict(row) if row else None


def get_latest_gate_state():
    conn = get_connection()

    cursor = conn.execute("""
        SELECT *
        FROM gate_log
        ORDER BY id DESC
        LIMIT 1
    """)

    row = cursor.fetchone()
    conn.close()

    return dict(row) if row else None


def get_latest_column(table_name, column_name):
    conn = get_connection()

    cursor = conn.execute(f"""
        SELECT {column_name}, timestamp
        FROM {table_name}
        WHERE {column_name} IS NOT NULL
        ORDER BY id DESC
        LIMIT 1
    """)

    row = cursor.fetchone()
    conn.close()

    return dict(row) if row else None


def get_recent_events(limit=10):
    conn = get_connection()

    cursor = conn.execute("""
        SELECT *
        FROM event_log
        ORDER BY id DESC
        LIMIT ?
    """, (limit,))

    rows = [dict(row) for row in cursor.fetchall()]
    conn.close()

    return rows


def get_dashboard_state():
    parking = get_latest_column("parking_log", "parking_state")
    risk = get_latest_column("parking_log", "risk_state")
    front = get_latest_column("parking_log", "front_distance")
    rear = get_latest_column("parking_log", "rear_distance")
    side = get_latest_column("parking_log", "side_distance")
    gate = get_latest_gate_state()

    return {
        "parking_state": parking["parking_state"] if parking else "UNKNOWN",
        "risk_state": risk["risk_state"] if risk else "UNKNOWN",
        "gate_state": gate["gate_state"] if gate else "UNKNOWN",
        "distances": {
            "front": front["front_distance"] if front else None,
            "rear": rear["rear_distance"] if rear else None,
            "side": side["side_distance"] if side else None,
        },
        "last_updated": max(
            [
                item["timestamp"]
                for item in [parking, risk, front, rear, side, gate]
                if item and item.get("timestamp")
            ],
            default=None,
        ),
        "events": get_recent_events(),
    }
