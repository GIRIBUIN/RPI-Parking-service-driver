# Rpi #3 - Central Server & Digital Twin Node

Rpi #3 is responsible for:
- MQTT broker
- Database management
- Dashboard monitoring
- 2D Digital Twin visualization
- System state synchronization

---

# Main Features

- Receive parking state from Rpi #1
- Receive gate state from Rpi #2
- Store sensor/event logs
- Update Digital Twin dashboard in real time
- Provide web monitoring UI

---

# Components

| Component | Role |
|---|---|
| MQTT Broker | Communication between Raspberry Pis |
| SQLite DB | Sensor/event log storage |
| Web Dashboard | Real-time monitoring |
| Digital Twin | Virtual parking space visualization |

---

# Dashboard Features

## Parking State Monitoring

Display:
- EMPTY
- APPROACHING
- PARKING
- PARKED

---

## Warning State Monitoring

Display:
- SAFE
- WARNING
- DANGER

Dashboard changes:
- Color updates
- Status text updates
- Real-time sensor value display

---

## Gate Monitoring

Display:
- OPEN
- CLOSED

---

# Digital Twin

The Digital Twin dashboard synchronizes:
- Real parking state
- Vehicle movement
- Gate status
- Warning state

Example:
```text
Vehicle approaches
→ Dashboard vehicle object moves

Danger state detected
→ Dashboard changes to warning color

Gate closes
→ Dashboard gate state updates
```

---

# MQTT Topics

## Subscribe

| Topic | Description |
|---|---|
| parking/rpi1/status | parking state |
| parking/rpi1/distance | sensor distances |
| parking/rpi1/risk | SAFE/WARNING/DANGER |
| parking/rpi1/event | parking events |
| parking/rpi2/gate | gate state |
| parking/rpi2/event | gate and exit request events |

---

# Database

SQLite is used to store:
- Parking events
- Distance logs
- Warning states
- Gate states
- Exit events

---

# Example Database Tables

## parking_log

| Column | Description |
|---|---|
| timestamp | event time |
| parking_state | current parking state |
| risk_state | SAFE/WARNING/DANGER |
| front_distance | front ultrasonic value |
| rear_distance | rear ultrasonic value |
| side_distance | side ultrasonic value |

---

## gate_log

| Column | Description |
|---|---|
| timestamp | event time |
| gate_state | OPEN/CLOSED |

---

# Example Flow

1. Rpi #1 publishes parking status
2. Rpi #3 receives MQTT message
3. Sensor/event data stored in SQLite
4. Dashboard updates in real time
5. Rpi #2 publishes gate state and exit events
6. Rpi #3 stores gate/exit events
7. User monitors parking state through web UI
