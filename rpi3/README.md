# RPI3 - Parking Monitoring Server

RPI3는 스마트 주차장 시스템의 중앙 서버 역할을 담당한다.

주요 역할은 다음과 같다.

- MQTT Broker 실행
- RPI1/RPI2 MQTT 메시지 수신
- 주차 슬롯 상태 저장
- 게이트 상태 저장
- 입차/출차 이벤트 로그 저장
- Web Dashboard 제공
- 주차장 상태 실시간 모니터링

---

# System Role

```text
RPI1: Parking Slot Sensing Node
- Slot 1 점유 상태 감지
- Slot 2 점유 상태 감지
- Parking Lot 상태 판단

RPI2: Gate Control Node
- 입구 차량 접근 감지
- 바리게이트 open/close
- 출차 버튼 이벤트 처리

RPI3: Central Server
- MQTT Broker
- SQLite DB
- Dashboard Monitoring
````

---

# Features

* MQTT topic subscribe
* Slot 1 / Slot 2 점유 상태 저장
* Slot별 초음파 거리값 저장
* Parking Lot 상태 저장
* Gate OPEN/CLOSED 상태 저장
* ENTRY_OPEN / EXIT_OPEN 이벤트 저장
* Dashboard에서 주차장 상태 시각화
* Dashboard에서 MQTT over WebSocket 직접 구독

---

# Repository Structure

```text
rpi3/
├── README.md
├── requirements.txt
├── app.py
├── mqtt_handler.py
├── mosquitto-websocket.conf
├── db.py
├── schema.sql
├── simulate_parking.ps1
├── templates/
│   └── index.html
└── static/
    ├── dashboard.js
    └── style.css
```

---

# File Description

## app.py

Flask Web Server.

역할:

* Dashboard HTML 제공
* 초기 Dashboard 상태 조회용 `/api/state` API 제공
* DB 초기화

실행:

```bash
python app.py
```

접속:

```text
http://localhost:5000
```

---

## mqtt_handler.py

MQTT Subscriber.

역할:

* `parking/#` topic subscribe
* topic별 payload 파싱
* SQLite DB 저장

실행:

```bash
python mqtt_handler.py
```

---

## db.py

SQLite DB 접근 함수 모음.

역할:

* DB 연결
* schema.sql 실행
* slot/gate/event 로그 저장
* Dashboard 상태 조회

---

## schema.sql

SQLite table 정의.

생성 테이블:

* `slot_log`
* `lot_log`
* `gate_log`
* `event_log`

---

## simulate_parking.ps1

RPI1/RPI2 없이 RPI3를 테스트하기 위한 MQTT mock publish 스크립트.

실행:

```powershell
.\simulate_parking.ps1
```

---

# MQTT Topics

## Subscribe Topics

RPI3는 전체 parking topic을 구독한다.

```text
parking/#
```

실제 처리 topic은 다음과 같다.

| Topic                   | Publisher | Subscriber | Payload | Description    |
| ----------------------- | --------- | ---------- | ------- | -------------- |
| `parking/rpi1/slot`     | RPI1      | RPI3       | JSON    | Slot 1/2 점유 상태 |
| `parking/rpi1/distance` | RPI1      | RPI3       | JSON    | Slot 1/2 거리값   |
| `parking/rpi1/lot`      | RPI1      | RPI2, RPI3 | TEXT    | 전체 주차장 상태      |
| `parking/rpi2/gate`     | RPI2      | RPI3       | TEXT    | 게이트 상태         |
| `parking/rpi2/event`    | RPI2      | RPI3       | JSON    | 입차/출차 이벤트 |

---

# Payload Format

## parking/rpi1/slot

```json
{
  "slot1": "OCCUPIED",
  "slot2": "EMPTY"
}
```

가능 값:

```text
EMPTY
OCCUPIED
```

---

## parking/rpi1/distance

```json
{
  "slot1": 12,
  "slot2": 48,
  "unit": "cm"
}
```

---

## parking/rpi1/lot

```text
AVAILABLE
```

또는:

```text
FULL
```

---

## parking/rpi2/gate

```text
OPEN
```

또는:

```text
CLOSED
```

---

## parking/rpi2/event

```json
{
  "event": "ENTRY_OPEN",
  "reason": "vehicle_detected"
}
```

```json
{
  "event": "EXIT_OPEN",
  "reason": "switch_pressed"
}
```

# State Definition

## Slot State

| State      | Description       |
| ---------- | ----------------- |
| `EMPTY`    | 주차 슬롯에 차량 없음      |
| `OCCUPIED` | 주차 슬롯이 차량에 의해 점유됨 |

---

## Parking Lot State

| State       | Description    |
| ----------- | -------------- |
| `AVAILABLE` | 하나 이상의 빈 슬롯 있음 |
| `FULL`      | 모든 슬롯이 점유됨     |

---

## Gate State

| State    | Description |
| -------- | ----------- |
| `OPEN`   | 바리게이트 열림    |
| `CLOSED` | 바리게이트 닫힘    |

---

## Gate Event

| Event        | Description         |
| ------------ | ------------------- |
| `ENTRY_OPEN` | 입구 차량 접근으로 바리게이트 열림 |
| `EXIT_OPEN`  | 출차 버튼 입력으로 바리게이트 열림 |

---

# Database Schema

## slot_log

Slot 1/2의 점유 상태와 거리값을 저장한다.

| Column           | Description |
| ---------------- | ----------- |
| `id`             | 로그 ID       |
| `timestamp`      | 저장 시각       |
| `slot1_state`    | Slot 1 상태   |
| `slot2_state`    | Slot 2 상태   |
| `slot1_distance` | Slot 1 거리값  |
| `slot2_distance` | Slot 2 거리값  |

---

## lot_log

전체 주차장 상태를 저장한다.

| Column      | Description      |
| ----------- | ---------------- |
| `id`        | 로그 ID            |
| `timestamp` | 저장 시각            |
| `lot_state` | AVAILABLE / FULL |

---

## gate_log

게이트 상태를 저장한다.

| Column       | Description   |
| ------------ | ------------- |
| `id`         | 로그 ID         |
| `timestamp`  | 저장 시각         |
| `gate_state` | OPEN / CLOSED |

---

## event_log

입차/출차 이벤트를 저장한다.

| Column       | Description     |
| ------------ | --------------- |
| `id`         | 로그 ID           |
| `timestamp`  | 저장 시각           |
| `topic`      | MQTT topic      |
| `event_type` | 이벤트 종류          |
| `reason`     | 이벤트 발생 이유       |
| `payload`    | 원본 MQTT payload |

---

# Install

## Python dependencies

```bash
pip install -r requirements.txt
```

`requirements.txt` 예시:

```text
flask
paho-mqtt
```

---

# Run

## 1. MQTT Broker 실행

Windows에서 Mosquitto 설치 후:

```powershell
cd rpi3
& "C:\Program Files\Mosquitto\mosquitto.exe" -c .\mosquitto-websocket.conf -v
```

Linux/Raspberry Pi:

```bash
cd rpi3
mosquitto -c mosquitto-websocket.conf -v
```

---

## 2. MQTT Handler 실행

```bash
python mqtt_handler.py
```

정상 실행 시:

```text
Subscribed to parking/#
```

---

## 3. Dashboard Server 실행

```bash
python app.py
```

브라우저 접속:

```text
http://localhost:5000
```

Dashboard는 처음 로딩할 때만 `/api/state`로 DB의 최신 상태를 가져오고, 이후에는 브라우저가 `ws://<RPI3_IP>:9001`로 MQTT topic을 직접 구독해 화면을 갱신한다.

---

# Mock Test

RPI1/RPI2 없이 MQTT 메시지를 직접 publish하여 테스트할 수 있다.

## Slot 상태 테스트

```powershell
mosquitto_pub -h localhost -t parking/rpi1/slot -m "{\"slot1\":\"OCCUPIED\",\"slot2\":\"EMPTY\"}"
```

## Distance 테스트

```powershell
mosquitto_pub -h localhost -t parking/rpi1/distance -m "{\"slot1\":12,\"slot2\":48,\"unit\":\"cm\"}"
```

## Parking Lot 상태 테스트

```powershell
mosquitto_pub -h localhost -t parking/rpi1/lot -m "AVAILABLE"
```

## Gate 상태 테스트

```powershell
mosquitto_pub -h localhost -t parking/rpi2/gate -m "OPEN"
```

## Event 테스트

```powershell
mosquitto_pub -h localhost -t parking/rpi2/event -m "{\"event\":\"ENTRY_OPEN\",\"reason\":\"vehicle_detected\"}"
```

---

# Dashboard API

## GET /api/state

초기 Dashboard 상태를 JSON으로 반환한다.

응답 예시:

```json
{
  "slot": {
    "slot1": "OCCUPIED",
    "slot2": "EMPTY"
  },
  "distance": {
    "slot1": 12,
    "slot2": 48,
    "unit": "cm"
  },
  "lot_state": "AVAILABLE",
  "gate_state": "OPEN",
  "last_updated": "2026-05-29 12:00:00",
  "events": []
}
```

---

# Real-time Strategy

본 시스템은 hard real-time 제어 시스템이 아니라 주차장 상태를 실시간에 가깝게 보여주는 near real-time monitoring 시스템이다.

실시간성 확보 방식:

1. RPI3는 MQTT 메시지를 수신하는 즉시 SQLite에 저장한다.
2. Dashboard는 최초 로딩 시 `/api/state`를 한 번 호출해 DB의 최신 상태를 복원한다.
3. 이후 Dashboard는 MQTT over WebSocket으로 `parking/rpi1/slot`, `parking/rpi1/distance`, `parking/rpi1/lot`, `parking/rpi2/gate`, `parking/rpi2/event`를 직접 subscribe한다.
4. 새 MQTT 메시지가 도착하면 브라우저 콜백에서 화면을 즉시 갱신한다.

---

# Notes

* RPI3는 하드웨어 센서/액추에이터를 직접 제어하지 않는다.
* RPI3는 MQTT, DB, Dashboard Monitoring에 집중한다.
* 게이트 제어는 RPI2가 담당한다.
* 슬롯 점유 판단은 RPI1이 담당한다.
* RPI3는 각 노드에서 발생한 상태와 이벤트를 저장하고 시각화한다.
## mosquitto-websocket.conf

Mosquitto Broker에서 일반 MQTT와 브라우저용 MQTT over WebSocket을 함께 열기 위한 설정 예시.

```text
listener 1883
protocol mqtt

listener 9001
protocol websockets
```

RPI1/RPI2는 `1883` 포트로 publish/subscribe하고, Dashboard 브라우저는 `9001` 포트로 MQTT topic을 직접 subscribe한다.

---
