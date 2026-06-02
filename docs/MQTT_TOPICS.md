# MQTT Topics

스마트 주차장 시스템의 MQTT 토픽 구조를 정의한다.

```text
parking/
├── rpi1/
│   ├── slot
│   ├── distance
│   └── lot
└── rpi2/
    ├── gate
    └── event
```

---

## Topic Ownership

| Topic | Publisher | Subscriber | 용도 |
|---|---|---|---|
| `parking/rpi1/slot` | RPI1 | RPI3, Dashboard | Slot 1, Slot 2 점유 상태 |
| `parking/rpi1/distance` | RPI1 | RPI3, Dashboard | Slot 1, Slot 2 초음파 거리값 |
| `parking/rpi1/lot` | RPI1 | RPI2, RPI3, Dashboard | 전체 주차장 상태 |
| `parking/rpi2/gate` | RPI2 | RPI3, Dashboard | 현재 게이트 상태 |
| `parking/rpi2/event` | RPI2 | RPI3, Dashboard | 진입, 출차 이벤트 |

---

## State Values

### Slot

| State | Description |
|---|---|
| `EMPTY` | 차량 없음 |
| `OCCUPIED` | 차량 주차 완료, 슬롯 점유 |

### Parking Lot

| State | Description |
|---|---|
| `AVAILABLE` | 빈 슬롯 있음 |
| `FULL` | 모든 슬롯 점유 |

### Gate

| State | Description |
|---|---|
| `OPEN` | 게이트 열림 |
| `CLOSED` | 게이트 닫힘 |

### Gate Event

| Event | Reason | Description |
|---|---|---|
| `ENTRY_OPEN` | `vehicle_detected` | 입구 차량 접근 감지로 게이트 open |
| `EXIT_OPEN` | `switch_pressed` | 출차 버튼 입력으로 게이트 open |

---

## Event Policy

- RPI1은 슬롯 초음파 센서 2개를 순차 측정한다.
- RPI1은 슬롯별 거리값과 `EMPTY`/`OCCUPIED` 상태를 판단해 발행한다.
- RPI1은 Slot 1, Slot 2가 모두 `OCCUPIED`이면 `FULL`, 하나라도 비어 있으면 `AVAILABLE`을 발행한다.
- RPI2는 `parking/rpi1/lot`을 구독해 만차 LED 표시 여부를 결정한다.
- RPI2는 입구 초음파 센서로 차량 접근을 감지하면 게이트를 `OPEN`하고 `ENTRY_OPEN` 이벤트를 발행한다.
- RPI2는 출차 스위치 입력을 감지하면 게이트를 `OPEN`하고 `EXIT_OPEN` 이벤트를 발행한다.
- RPI2는 게이트 open 후 timer가 만료되면 `parking/rpi2/gate`에 `CLOSED`를 발행한다.
- RPI2는 차량 접근이 새로 감지되면 close timer를 갱신한다.
- RPI3는 RPI1/RPI2의 상태와 이벤트를 구독해 DB와 Dashboard에 반영한다.
- Dashboard는 MQTT over WebSocket으로 동일 topic을 직접 구독해 화면을 갱신한다.

---

## Example Messages

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
