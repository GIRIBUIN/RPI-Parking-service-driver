# RPI2 바리게이트 제어 노드 구현 계획

## Context
Digital Twin 스마트 주차 시스템 팀 프로젝트에서 RPI2(바리게이트 제어 노드)를 담당.
`docs/dev_schedule.md`의 4주 일정에 따라 서보 모터 게이트 제어, Switch 출차 버튼, LED 상태 표시, MQTT 통신을 구현한다.
현재 `rpi2/` 폴더에는 `README.md`만 있고 코드가 없는 초기 상태.

---

## 파일 구조 (신규 생성 대상)

```
rpi2/
├── README.md                (기존)
├── PLAN.md                  (이 파일)
├── Makefile                 (빌드 설정)
├── config.h                 (핀 번호, PWM, MQTT 상수)
├── gate_controller.h        (서보 모터 + LED 인터페이스)
├── gate_controller.c        (서보 모터 + LED 구현)
├── switch_handler.h         (Switch 인터럽트 인터페이스)
├── switch_handler.c         (Switch 인터럽트 + 디바운싱 구현)
├── mqtt_client.h            (MQTT 인터페이스)
├── mqtt_client.c            (paho-mqtt C 클라이언트 구현)
└── main.c                   (진입점, 전체 루프)
```

---

## GPIO 핀 할당 (BCM)

| 핀 | BCM | 물리 핀 | 역할 |
|----|-----|---------|------|
| 서보 모터 | 18 | 12 | 하드웨어 PWM0 지원 |
| LED | 24 | 18 | 상태 표시 (220Ω 저항 필요) |
| Switch | 23 | 16 | 출차 버튼 (내부 풀업) |

---

## 서보 PWM 파라미터

- 주파수: 50Hz (20ms 주기, 서보 표준)
- OPEN 듀티: 2.5% (0.5ms → 0도)
- CLOSED 듀티: 12.0% (2.4ms → ~180도)
- 이동 대기: 0.5초 후 `ChangeDutyCycle(0)` → 과부하 방지

---

## MQTT 토픽

| 방향 | 토픽 | 내용 |
|------|------|------|
| 구독 | `parking/control/gate` | OPEN / CLOSE 명령 |
| 발행 | `parking/rpi2/gate` | OPEN / CLOSED 상태 (retain=True) |
| 발행 | `parking/rpi2/event` | exit_requested 이벤트 |

---

## 핵심 구현 사항

### gate_controller.py
- `GateController` 클래스
- `setup()`, `open_gate()`, `close_gate()`, `get_state()`, `cleanup()`
- 이미 같은 상태면 중복 동작 건너뜀
- `threading.Lock()` 내부 보유 → MQTT/Switch 동시 호출 레이스 컨디션 방지

### switch_handler.py
- `SwitchHandler(on_press_callback)` 클래스
- 2단계 디바운싱:
  1. `GPIO.add_event_detect(..., bouncetime=200)` (HW 레벨)
  2. `time.monotonic()` 기반 300ms 소프트웨어 필터 (SW 레벨)

### mqtt_client.py
- `MqttClient(on_gate_command)` 클래스
- `reconnect_delay_set(1, 30)` → paho 자동 지수 백오프 재연결
- `on_disconnect` → `_connected = False`
- **Failsafe**: 메인 루프에서 연결 끊김 10초 지속 시 `gate.open_gate()` 강제 호출

### main.py
- `SIGINT/SIGTERM` 핸들러 → cleanup 보장
- 시작 시 `gate.open_gate()` + `publish_gate_state("OPEN")`
- `while True: failsafe_check_loop(); time.sleep(1)`

---

## 4주차 개발 일정

| 주차 | 핵심 작업 | 완료 기준 |
|------|-----------|-----------|
| 1주차 | 서보/LED/Switch 개별 스크립트 작성 및 하드웨어 테스트 | OPEN/CLOSED 각도 확정, 버튼 인터럽트 동작 확인 |
| 2주차 | MQTT 연결 + 전체 시나리오 통합 (`main.py`) | `mosquitto_pub`으로 CLOSE 발행 시 게이트 닫힘, Switch 클릭 시 열림+이벤트 발행 |
| 3주차 | 디바운싱 처리, 물리 모형 고정, RPI1과 통합 테스트 | 버튼 연속 클릭 시 이벤트 중복 없음, 진입→주차→출차→EMPTY 사이클 성공 |
| 4주차 | 모터 과부하 방지 (PWM 차단), Failsafe 로직 검증 | 브로커 종료 시 게이트 OPEN 유지, 장시간(1시간) 오류 없음 |

---

## 하드웨어 주의사항

- 서보 VCC는 외부 5V 또는 RPi 5V 핀에서 공급 (GPIO 3.3V 직결 금지)
- LED에 220Ω 저항 직렬 연결 필수
- Switch는 `GPIO.PUD_UP` 내부 풀업 사용, FALLING 엣지 감지

---

## 검증 방법

```bash
# 브로커에 CLOSE 명령 발행
mosquitto_pub -h <RPI3_IP> -t "parking/control/gate" -m "CLOSE"

# RPI2 발행 토픽 모니터링
mosquitto_sub -h <RPI3_IP> -t "parking/rpi2/#" -v

# Failsafe 테스트
sudo systemctl stop mosquitto  # 브로커 종료 → 10초 후 게이트 OPEN 확인
```
