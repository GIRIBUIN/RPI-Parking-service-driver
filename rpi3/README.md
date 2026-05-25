# RPI3 - Central Server & Digital Twin Dashboard

RPI3 is responsible for:
- MQTT Broker
- MQTT message subscription
- Sensor/event logging
- SQLite database management
- Real-time dashboard monitoring
- 2D Digital Twin visualization

---

# Main Features

- Receive MQTT messages from RPI1 and RPI2
- Store parking/gate/event logs
- Provide real-time parking dashboard
- Synchronize physical parking state with digital twin

---

# System Role

RPI3 acts as the central server of the system.

```text
RPI1 (Sensor Node)
        ↓ MQTT

RPI3 (Broker + Dashboard + DB)

        ↑ MQTT
RPI2 (Gate Node)
```

---

# MQTT Topics

## Subscribe

| Topic | Description |
|---|---|
| parking/rpi1/status | parking state |
| parking/rpi1/distance | ultrasonic distances |
| parking/rpi1/risk | SAFE/WARNING/DANGER |
| parking/rpi2/gate | gate state |
| parking/rpi2/event | exit request event |

---

# Dashboard Features

- Parking state monitoring
- Gate state monitoring
- Distance visualization
- Risk state monitoring
- Real-time updates

---

# Parking State

| State | Description |
|---|---|
| EMPTY | no vehicle |
| APPROACHING | vehicle approaching |
| PARKING | vehicle entering |
| PARKED | parking completed |
| EXITING | vehicle exiting |

---

# Risk State

| State | Description |
|---|---|
| SAFE | safe distance |
| WARNING | near threshold |
| DANGER | collision risk |

---

# Repository Structure

```text
rpi3/
├── README.md
├── requirements.txt
├── app.py
├── mqtt_handler.py
├── state_manager.py
├── db.py
├── schema.sql
├── templates/
│   └── index.html
└── static/
    ├── style.css
    └── dashboard.js
```

---

# Technologies

- Python
- Flask
- MQTT (Mosquitto)
- SQLite
- HTML/CSS/JavaScript

---

# Install

```bash
pip install -r requirements.txt
```

---

# Run MQTT Broker

```bash
mosquitto
```

---

# Run Dashboard Server

```bash
python app.py
```

---

# Run MQTT Subscriber

```bash
python mqtt_handler.py
```

---

# Example MQTT Test

```bash
mosquitto_pub -h localhost \
-t parking/rpi1/status \
-m "PARKING"
```

---

# Future Work

- Multi parking slot support
- Camera integration
- Mobile dashboard
- 3D Digital Twin
