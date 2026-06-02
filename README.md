# 주차장 점유 상태 모니터링

초음파 센서를 통해 주차 슬롯의 점유 상태를 판단하고, 주차장 입구 바리게이트를 제어하며, 중앙 서버에서 MQTT/DB/Dashboard를 통해 전체 상태를 모니터링하는 IoT 기반 스마트 주차장 시스템이다.

주차장은 입구 바리게이트 1개와 내부 주차 슬롯 2개로 구성한다. 슬롯이 모두 점유된 경우에는 만차 상태를 LED와 Dashboard로 안내하고, 차량은 내부 회차 공간을 통해 출구 방향으로 이동한다.

---

## 프로젝트 목표

### 감지
- Slot 1, Slot 2의 점유 상태 감지
- 초음파 센서 거리값 기반으로 `EMPTY`, `OCCUPIED` 판단

### 경고
- 주차 슬롯 만차 상태를 LED로 알림
- 출차 이벤트 상태를 LED/Buzzer로 알림

### 제어
- 주차장 입구 바리게이트 제어
- 입구 초음파 센서가 차량 접근을 감지하면 바리게이트 open
- 일정 시간이 지나면 바리게이트 close
- 물체가 새로 접근하면 close timer 갱신
- 출차 시에는 스위치 입력으로 바리게이트 open 후 일정 시간 뒤 close

### 모니터링
- Slot 1, Slot 2 점유 상태 표시
- 전체 주차장 상태 표시
- 바리게이트 상태 표시
- 최근 이벤트 로그 표시

---

## 시스템 구조

### Rpi #1

담당: 주차 점유 판단

역할:
- 슬롯별 초음파 센싱
- 슬롯 점유 상태 판단
- 전체 주차장 상태 판단
- 슬롯별 LED 제어
- MQTT로 슬롯 상태, 거리값, 전체 주차장 상태 발행

센서/액추에이터:
- Ultrasonic #1: Slot 1 점유 센싱
- Ultrasonic #2: Slot 2 점유 센싱
- Slot 1 LED: `EMPTY` 소등, `OCCUPIED` 점등
- Slot 2 LED: `EMPTY` 소등, `OCCUPIED` 점등

### Rpi #2

담당: 바리게이트 제어

역할:
- 입구 차량 접근 감지
- 주차장 만차 상태 수신
- 게이트 open/close 제어
- 출차 요청 스위치 이벤트 처리
- LED/Buzzer로 만차 및 출차 이벤트 표시
- MQTT로 게이트 상태와 이벤트 발행

센서/액추에이터:
- Ultrasonic #3: 주차장 입구 차량 접근 감지
- Switch: 출차 요청 버튼
- Servo Motor: 게이트 open/close
- LED: 만차 이벤트 상태 표시
- LED/Buzzer: 출차 이벤트 상태 표시

### Rpi #3

담당: 중앙 서버

역할:
- MQTT Broker 실행
- RPI1/RPI2 메시지 수신
- 슬롯 점유 상태 저장
- 게이트 상태 저장
- 이벤트 로그 저장
- Dashboard에 주차장 상태 표시

Dashboard 표시 항목:
- Slot 1 상태
- Slot 2 상태
- Parking Lot 상태: `AVAILABLE` / `FULL`
- Gate 상태: `OPEN` / `CLOSED`
- 최근 이벤트 로그

---

## 상태 정의

### Slot

| State | Description |
|---|---|
| EMPTY | 차량 없음 |
| OCCUPIED | 주차 완료, 슬롯 점유 |

### Parking Lot

| State | Description |
|---|---|
| AVAILABLE | 빈 슬롯 있음 |
| FULL | 모든 슬롯 점유 |

### Gate State

| State | Description |
|---|---|
| OPEN | 게이트 열림 |
| CLOSED | 게이트 닫힘 |

### Gate Event

| Event | Description |
|---|---|
| ENTRY_OPEN | 차량 진입 감지로 인한 게이트 open |
| EXIT_OPEN | 출차 스위치 입력으로 인한 게이트 open |
| AUTO_CLOSE | timer 만료로 인한 게이트 close |

---

## 동작 시나리오

### 정상 입차 시나리오

1. 차량이 주차장 입구에 접근한다.
2. RPI2의 입구 초음파 센서가 차량 접근을 감지한다.
3. RPI2는 현재 주차장 상태를 확인한다.
4. 빈 주차 슬롯이 있는 경우, 입구 바리게이트를 open한다.
5. 차량이 주차장 내부로 진입한다.
6. 일정 시간이 지나면 RPI2는 바리게이트를 close한다.
7. 차량이 주차 슬롯에 진입하면 RPI1의 슬롯 초음파 센서가 차량을 감지한다.
8. 차량이 슬롯 내부에 일정 거리 이상 들어온 상태가 지속되면 해당 슬롯을 `OCCUPIED` 상태로 판단한다.
9. RPI1은 슬롯 점유 상태와 거리값을 MQTT로 RPI3에 전송한다.
10. RPI3는 수신한 데이터를 DB에 저장하고 Dashboard에 반영한다.
11. Dashboard에는 Slot 1, Slot 2의 점유 상태와 전체 주차장 상태가 표시된다.

### 만차 시나리오

1. Slot 1과 Slot 2가 모두 `OCCUPIED` 상태가 되면 주차장 상태는 `FULL`로 변경된다.
2. RPI1은 전체 주차장 상태를 `FULL`로 판단하고 MQTT를 통해 RPI3와 RPI2에 전송한다.
3. RPI2는 입구 LED를 통해 만차 상태를 표시한다.
4. 이후 차량이 주차장 입구에 접근하면 RPI2의 입구 초음파 센서가 차량을 감지한다.
5. 현실 주차장처럼 바리게이트는 open되지만, LED와 Dashboard를 통해 주차 가능한 공간이 없음을 안내한다.
6. 차량은 주차하지 않고 내부 회차 공간을 통해 출구 방향으로 이동한다.
7. 운전자가 출차 버튼을 누르면 RPI2는 출차 이벤트를 발생시키고 바리게이트를 open한다.
8. 일정 시간이 지나면 RPI2는 바리게이트를 close한다.
9. RPI3는 만차 상태, 바리게이트 상태, 출차 이벤트를 DB에 저장하고 Dashboard에 표시한다.

### 출차 시나리오

1. 주차된 차량이 주차 슬롯에서 빠져나가기 시작한다.
2. RPI1의 슬롯 초음파 센서는 차량과의 거리가 멀어지는 것을 감지한다.
3. 차량이 슬롯에서 일정 거리 이상 벗어난 상태가 지속되면 해당 슬롯을 `EMPTY` 상태로 판단한다.
4. RPI1은 변경된 슬롯 상태를 MQTT로 RPI3에 전송한다.
5. 전체 주차장에 빈 슬롯이 생기면 주차장 상태는 `AVAILABLE`로 변경된다.
6. 차량이 출구에 도착하면 운전자는 출차 버튼을 누른다.
7. RPI2는 출차 버튼 입력을 감지하고 바리게이트를 open한다.
8. RPI2는 `EXIT_OPEN` 이벤트와 게이트 상태를 MQTT로 RPI3에 전송한다.
9. 차량이 출차한 뒤 일정 시간이 지나면 RPI2는 바리게이트를 close한다.
10. RPI3는 출차 이벤트, 게이트 상태, 슬롯 상태 변화를 DB에 저장하고 Dashboard에 반영한다.

---

## MQTT Topics

| Topic | Publisher | Subscriber | 용도 |
|---|---|---|---|
| `parking/rpi1/slot` | RPI1 | RPI3 | slot 별 주차 점유 상태 |
| `parking/rpi1/distance` | RPI1 | RPI3 | slot 별 거리값 |
| `parking/rpi1/lot` | RPI1 | RPI2, RPI3 | 전체 주차장 만차 여부 |
| `parking/rpi2/gate` | RPI2 | RPI3 | 게이트 상태 전송 |
| `parking/rpi2/event` | RPI2 | RPI3 | 진입/출차/자동닫힘 이벤트 |

### `parking/rpi1/slot`

```json
{
  "slot1": "OCCUPIED",
  "slot2": "EMPTY"
}
```

### `parking/rpi1/distance`

```json
{
  "slot1": 12,
  "slot2": 48,
  "unit": "cm"
}
```

### `parking/rpi1/lot`

```text
AVAILABLE
FULL
```

### `parking/rpi2/gate`

```text
OPEN
CLOSED
```

### `parking/rpi2/event`

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

```json
{
  "event": "AUTO_CLOSE",
  "reason": "timer_expired"
}
```

---

## 초음파 센서

- 모델: HC-SR04
- Datasheet: https://www.elecrow.com/download/HC_SR04%20Datasheet.pdf
- 세 초음파 센서를 동시에 측정하지 않는다.
- 초음파 간섭을 줄이기 위해 센서는 순차적으로 측정한다.

---

## 물리 모형

- 주차장 모형: 박스 상자로 육면체 구성
- 입구: 바리게이트
- 내부: 주차 슬롯 2개
- 회차 공간: 만차 시 차량이 출구 방향으로 이동할 수 있는 공간

---

## Repository Structure

```text
parking/
├── README.md
├── docs/
├── rpi1/
├── rpi2/
└── rpi3/
```

---

## Technologies

- Raspberry Pi
- GPIO
- MQTT
- SQLite
- Dashboard
- Python
- C
