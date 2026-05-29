CREATE TABLE IF NOT EXISTS slot_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,

    slot1_state TEXT,
    slot2_state TEXT,

    slot1_distance REAL,
    slot2_distance REAL
);

CREATE TABLE IF NOT EXISTS lot_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,

    lot_state TEXT
);

CREATE TABLE IF NOT EXISTS gate_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,

    gate_state TEXT
);

CREATE TABLE IF NOT EXISTS event_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,

    topic TEXT NOT NULL,
    event_type TEXT,
    reason TEXT,
    payload TEXT
);