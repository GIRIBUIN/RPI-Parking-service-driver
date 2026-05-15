# Digital Twin Smart Parking System

IoT/CPS 기반 스마트 주차 공간 모니터링 시스템.

실제 주차 공간 상태를 초음파 센서를 통해 측정하고,
Digital Twin Dashboard에 실시간으로 반영하는 스마트 주차 시스템이다.

차량 접근 및 주차 상태를 감지하고,
LED/Buzzer를 통해 위험 상태를 경고하며,
Servo Motor를 이용해 바리게이트를 제어한다.

센서 데이터와 상태 정보는 MQTT를 통해 중앙 서버(Rpi #3)로 전송되며,
Dashboard에서 실시간 상태를 모니터링할 수 있다.

---

# Project Goal

- 차량 접근 및 주차 상태 실시간 감지
- 후방/측면 충돌 위험 경고
- 게이트 자동 제어
- 현실 공간과 Digital Twin 동기화
- 실시간 상태 모니터링 제공

---

# System Architecture

Rpi #1
- 주차 공간 센싱 + 상태 판단 + 경고

Rpi #2
- 바리게이트 제어 + 출차 버튼 처리

Rpi #3
- MQTT Broker + DB + Dashboard Monitoring

---

# Scenario

1. 차량이 주차 공간 입구에 접근
2. 초음파 센서를 통해 거리 측정
3. 위험 상태를 LED/Buzzer로 표시
4. 차량이 주차 완료 상태가 되면 게이트 닫힘
5. Dashboard(Digital Twin)에 상태 실시간 반영
6. 사용자가 출차 버튼(Switch) 입력
7. 게이트 열림
8. 차량 이탈 확인 후 EMPTY 상태 복귀

---

# Hardware Components

## Rpi #1
- Ultrasonic Sensor x3
- LED x1
- Buzzer x1

## Rpi #2
- Servo Motor x1
- Switch x1
- LED x1

## Rpi #3
- MQTT Broker
- SQLite DB
- Web Dashboard

---

# Ultrasonic Sensor Roles

## Ultrasonic #1
주차 공간 정면(입구)

역할:
- 차량 접근 감지

## Ultrasonic #2
주차 공간 후면

역할:
- 주차 완료 판단
- 후방 충돌 위험 감지

## Ultrasonic #3
주차 공간 측면(우측)

역할:
- 측면 충돌 위험 감지

---

# Warning State

## SAFE
- LED: 소등
- Buzzer: 무음

## WARNING
- LED: 깜빡임
- Buzzer: 간헐적 beep

## DANGER
- LED: 점등 유지
- Buzzer: 연속 경고음

---

# Parking State

| State | Description |
|---|---|
| EMPTY | 차량 없음 |
| APPROACHING | 차량 접근 중 |
| PARKING | 주차 중 |
| PARKED | 주차 완료 |

---

# Risk State

| State | Description |
|---|---|
| SAFE | 안전 거리 |
| WARNING | 경고할 만한 거리 |
| DANGER | 충돌 위험 거리 |

---

# Gate State

| State | Description |
|---|---|
| OPEN | 게이트 열림 |
| CLOSED | 게이트 닫힘 |

---

# MQTT Topics

MQTT topic 기준은 `docs/MQTT_TOPICS.md`에 정리한다.

핵심 흐름:
- Rpi #1은 주차 상태, 거리, 위험도, 주차 이벤트를 발행한다.
- Rpi #1이 `PARKED`를 판단하면 `parking/control/gate`로 `CLOSE` 명령을 발행한다.
- Rpi #2는 `parking/control/gate`를 구독해 게이트를 제어한다.
- Rpi #2는 게이트 상태를 `parking/rpi2/gate`로 발행한다.
- Rpi #2의 출차 버튼 이벤트는 `parking/rpi2/event`에 포함한다.
- Rpi #3는 Rpi #1, Rpi #2의 상태와 이벤트를 구독해 DB와 Dashboard에 반영한다.

---

# Repository Structure

```text
parking-project/
├── README.md
├── docs/
│   ├── ESS.drawio.png
│   ├── MQTT_TOPICS.md
│   └── 시나리오.pdf
├── rpi1/
├── rpi2/
└── rpi3/
```

---

# Technologies

- Raspberry Pi
- Linux Device Driver
- GPIO
- MQTT
- SQLite
- HTML/CSS/JavaScript
- Python
- C

---

# Physical Prototype

## Parking Space
- Width: 50cm
- Height: 30cm
- Wall Height: 15cm

## Car Model
- Length: 15cm
- Width: 8cm
- Height: 6cm

---

# Team Roles

## Rpi #1
Parking sensing and warning system

## Rpi #2
Barrier gate control

## Rpi #3
MQTT, DB, Dashboard Monitoring
