# Rpi #2 - Barrier Gate Control Node

Rpi #2 is responsible for:
- Barrier gate control
- Exit request handling
- Gate state management
- MQTT gate status publishing

---

# Hardware Components

| Device | Role |
|---|---|
| Servo Motor | Barrier gate open/close |
| Switch | Exit request button |
| LED | Gate state indication |

---

# Gate Logic

## Default State

Initial state:
```text
Gate OPEN
```

The barrier gate remains open until parking is completed.

---

## Gate Close

Condition:
```text
Rpi #1 publishes CLOSE to parking/control/gate
```

Action:
- Servo motor closes gate
- Gate state changes to CLOSED
- CLOSED state published to `parking/rpi2/gate`
- Gate close event published to `parking/rpi2/event`

---

## Gate Open

Condition:
```text
User presses exit button(Switch)
```

Action:
- Servo motor opens gate
- Gate state changes to OPEN
- OPEN state published to `parking/rpi2/gate`
- Exit request event published to `parking/rpi2/event`

---

# Gate State

| State | Description |
|---|---|
| OPEN | Gate opened |
| CLOSED | Gate closed |

---

# LED Logic

| Gate State | LED |
|---|---|
| OPEN | LED OFF |
| CLOSED | LED ON |

---

# MQTT Topics

## Subscribe

| Topic | Description |
|---|---|
| parking/control/gate | gate OPEN/CLOSE command from Rpi #1 |

---

## Publish

| Topic | Description |
|---|---|
| parking/rpi2/gate | current gate state |
| parking/rpi2/event | gate and exit request events |

---

# Example Flow

1. Rpi #1 detects parking completion
2. Rpi #1 publishes `CLOSE` to `parking/control/gate`
3. Rpi #2 receives the gate command
4. Servo motor closes gate
5. Gate `CLOSED` state published to `parking/rpi2/gate`
6. User presses exit button
7. Servo motor opens gate
8. Gate `OPEN` state published to `parking/rpi2/gate`
9. Exit request event published to `parking/rpi2/event`
