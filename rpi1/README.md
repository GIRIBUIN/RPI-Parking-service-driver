# Rpi #1 - Parking Sensor & Warning Node

Rpi #1 is responsible for:
- Parking space sensing
- Parking state 판단
- Collision risk detection
- LED/Buzzer warning output
- MQTT status publishing
- Gate close command publishing

---

# Hardware Components

| Device | Role |
|---|---|
| Ultrasonic Sensor #1 | Vehicle approach detection |
| Ultrasonic Sensor #2 | Parking completion + rear collision detection |
| Ultrasonic Sensor #3 | Side collision detection |
| LED | Warning state indication |
| Buzzer | Warning sound output |

---

# Ultrasonic Sensor Layout

## Ultrasonic #1
Location:
- Parking entrance(front)

Role:
- Detect vehicle approaching

---

## Ultrasonic #2
Location:
- Rear wall of parking space

Role:
- Determine parking completion
- Detect rear collision danger

---

## Ultrasonic #3
Location:
- Right-side wall of parking space

Role:
- Detect side collision danger

---

# Warning State Logic

## SAFE
Condition:
- Vehicle is in safe distance range

Action:
- LED OFF
- Buzzer OFF

---

## WARNING
Condition:
- Vehicle is near warning threshold

Action:
- LED blinking
- Intermittent buzzer beep

---

## DANGER
Condition:
- Collision risk detected

Action:
- LED always ON
- Continuous buzzer sound

---

# Parking State Logic

## EMPTY
No vehicle detected.

---

## APPROACHING
Vehicle detected near entrance.

Example:
```text
front_distance < threshold
```

---

## PARKING
Vehicle entering parking space.

Detected using:
- rear sensor distance changes

---

## PARKED
Parking completed.

Example:
```text
rear_distance < parked_threshold
```

Action:
- Publish `CLOSE` to `parking/control/gate`

---

## PARKED -> EMPTY
Vehicle departure detected after the gate opens.

Example:
```text
front_distance, rear_distance, side_distance return to empty range
```

Action:
- Publish `EMPTY` to `parking/rpi1/status`
- Publish departure event to `parking/rpi1/event`

---

# MQTT Topics

## Publish

| Topic | Description |
|---|---|
| parking/rpi1/status | parking state |
| parking/rpi1/distance | sensor distances |
| parking/rpi1/risk | SAFE/WARNING/DANGER |
| parking/rpi1/event | parking events |
| parking/control/gate | gate control command for Rpi #2 |

---

# Example Flow

1. Vehicle approaches parking entrance
2. Ultrasonic #1 detects vehicle
3. Distance decreases
4. Warning state updates
5. LED/Buzzer changes
6. MQTT status published
7. Ultrasonic #2 detects parking completion
8. `CLOSE` command published to `parking/control/gate`
9. Rpi #2 closes the gate
10. After exit and vehicle departure, Rpi #1 publishes `EMPTY`

---

# Important Notes

- Ultrasonic sensors must NOT measure simultaneously
- Sensors should measure sequentially
- Sequential measurement prevents ultrasonic interference
