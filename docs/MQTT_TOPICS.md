# MQTT Topics

This document defines the MQTT topic structure for the smart parking system.

```text
parking/
├── rpi1/
│   ├── status
│   ├── distance
│   ├── risk
│   └── event
│
├── rpi2/
│   ├── gate
│   └── event
│
└── control/
    └── gate
```

---

# Topic Ownership

| Topic | Publisher | Subscriber | Description |
|---|---|---|---|
| parking/rpi1/status | RPI1 | RPI3 | Parking state: EMPTY, APPROACHING, PARKING, PARKED |
| parking/rpi1/distance | RPI1 | RPI3 | Ultrasonic sensor distance values |
| parking/rpi1/risk | RPI1 | RPI3 | Risk state: SAFE, WARNING, DANGER |
| parking/rpi1/event | RPI1 | RPI3 | Parking-related events |
| parking/rpi2/gate | RPI2 | RPI3 | Current gate state: OPEN, CLOSED |
| parking/rpi2/event | RPI2 | RPI3 | Gate and exit request events |
| parking/control/gate | RPI1 | RPI2 | Gate control command: OPEN, CLOSE |

---

# Event Policy

- RPI1 publishes parking states and sensor/risk data.
- When RPI1 detects `PARKED`, it publishes `CLOSE` to `parking/control/gate`.
- RPI2 subscribes to `parking/control/gate` and controls the servo motor.
- RPI2 publishes the current gate state to `parking/rpi2/gate`.
- RPI2 publishes button and gate events to `parking/rpi2/event`.
- RPI3 only monitors and stores system data. It does not directly control the gate.

---

# Example Messages

## parking/rpi1/status

```text
PARKED
```

## parking/rpi1/distance

```json
{
  "front": 18.2,
  "rear": 7.5,
  "side": 12.0
}
```

## parking/rpi1/risk

```text
WARNING
```

## parking/rpi1/event

```text
parking_completed
```

## parking/control/gate

```text
CLOSE
```

## parking/rpi2/gate

```text
CLOSED
```

## parking/rpi2/event

```text
exit_requested
```
