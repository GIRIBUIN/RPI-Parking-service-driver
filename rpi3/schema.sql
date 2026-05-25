CREATE TABLE IF NOT EXISTS parking_log (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    parking_state TEXT,
    risk_state TEXT,
    front_distance REAL,
    rear_distance REAL,
    side_distance REAL
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
    payload TEXT
);
