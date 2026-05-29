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


def insert_slot_log(
    slot1_state=None,
    slot2_state=None,
    slot1_distance=None,
    slot2_distance=None
):
    conn = get_connection()

    conn.execute("""
        INSERT INTO slot_log (
            slot1_state,
            slot2_state,
            slot1_distance,
            slot2_distance
        )
        VALUES (?, ?, ?, ?)
    """, (
        slot1_state,
        slot2_state,
        slot1_distance,
        slot2_distance
    ))

    conn.commit()
    conn.close()


def insert_lot_log(lot_state):
    conn = get_connection()

    conn.execute("""
        INSERT INTO lot_log (
            lot_state
        )
        VALUES (?)
    """, (lot_state,))

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


def insert_event_log(topic, event_type=None, reason=None, payload=None):
    conn = get_connection()

    conn.execute("""
        INSERT INTO event_log (
            topic,
            event_type,
            reason,
            payload
        )
        VALUES (?, ?, ?, ?)
    """, (
        topic,
        event_type,
        reason,
        payload
    ))

    conn.commit()
    conn.close()


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


def get_latest_slot_state():
    conn = get_connection()

    cursor = conn.execute("""
        SELECT slot1_state, slot2_state, timestamp
        FROM slot_log
        WHERE slot1_state IS NOT NULL
           OR slot2_state IS NOT NULL
        ORDER BY id DESC
        LIMIT 1
    """)

    row = cursor.fetchone()
    conn.close()

    return dict(row) if row else None


def get_latest_slot_distance():
    conn = get_connection()

    cursor = conn.execute("""
        SELECT slot1_distance, slot2_distance, timestamp
        FROM slot_log
        WHERE slot1_distance IS NOT NULL
           OR slot2_distance IS NOT NULL
        ORDER BY id DESC
        LIMIT 1
    """)

    row = cursor.fetchone()
    conn.close()

    return dict(row) if row else None


def get_latest_lot_state():
    conn = get_connection()

    cursor = conn.execute("""
        SELECT lot_state, timestamp
        FROM lot_log
        WHERE lot_state IS NOT NULL
        ORDER BY id DESC
        LIMIT 1
    """)

    row = cursor.fetchone()
    conn.close()

    return dict(row) if row else None


def get_latest_gate_state():
    conn = get_connection()

    cursor = conn.execute("""
        SELECT gate_state, timestamp
        FROM gate_log
        WHERE gate_state IS NOT NULL
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
    slot_state = get_latest_slot_state()
    slot_distance = get_latest_slot_distance()
    lot = get_latest_lot_state()
    gate = get_latest_gate_state()

    timestamps = [
        item["timestamp"]
        for item in [slot_state, slot_distance, lot, gate]
        if item and item.get("timestamp")
    ]

    return {
        "slot": {
            "slot1": slot_state["slot1_state"] if slot_state else "UNKNOWN",
            "slot2": slot_state["slot2_state"] if slot_state else "UNKNOWN",
        },
        "distance": {
            "slot1": slot_distance["slot1_distance"] if slot_distance else None,
            "slot2": slot_distance["slot2_distance"] if slot_distance else None,
            "unit": "cm",
        },
        "lot_state": lot["lot_state"] if lot else "UNKNOWN",
        "gate_state": gate["gate_state"] if gate else "UNKNOWN",
        "last_updated": max(timestamps, default=None),
        "events": get_recent_events(),
    }