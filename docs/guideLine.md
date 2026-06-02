# 제작 가이드라인

---

## 1. 시스템 아키텍처 및 기술 스택

기획서에 명시된 3대의 Raspberry Pi를 다음 역할로 분리한다.

- **Rpi #1**: 주차 슬롯 점유 판단 노드
- **Rpi #2**: 입구 바리게이트 제어 노드
- **Rpi #3**: MQTT Broker, DB, Dashboard 중앙 서버

기술 스택:
- **하드웨어 및 OS**: Raspberry Pi 3대, Raspberry Pi OS
- **Rpi #1 디바이스 제어**: Python 또는 C, GPIO 직접 제어
- **Rpi #2 디바이스 제어**: C, `/sys/class/gpio/`, `/sys/class/pwm/` 기반 GPIO/PWM 제어
- **통신 프로토콜**: MQTT
- **Broker**: Eclipse Mosquitto, Rpi #3에 설치
- **Rpi #3 백엔드 및 DB**: Python FastAPI + SQLite
- **Dashboard**: HTML/CSS/JavaScript 또는 React 기반 웹 대시보드

---

## 1.1 wiringPi 라이브러리 사용 금지

`wiringPi`는 사용하지 않는다.

이유:
- 외부 라이브러리 의존성 최소화
- 커널 표준 인터페이스(`/sys/class/gpio/`, `/sys/class/pwm/`) 직접 사용
- 유지보수성 및 독립성 확보

GPIO는 `/sys/class/gpio/` 인터페이스로, PWM은 `/sys/class/pwm/` 인터페이스로 직접 제어한다.

---

## 2. 디바이스별 구현 가이드라인

### Rpi #1: 주차 슬롯 점유 판단 노드

역할:
- Slot 1, Slot 2 초음파 거리 측정
- 슬롯별 `EMPTY`/`OCCUPIED` 상태 판단
- 전체 주차장 `AVAILABLE`/`FULL` 상태 판단
- 슬롯별 LED 제어
- MQTT로 슬롯 상태, 거리값, 전체 주차장 상태 발행

하드웨어:
- Ultrasonic #1: Slot 1 점유 센싱
- Ultrasonic #2: Slot 2 점유 센싱
- Slot 1 LED: `OCCUPIED` 점등, `EMPTY` 소등
- Slot 2 LED: `OCCUPIED` 점등, `EMPTY` 소등

상태 판별:
- 차량이 슬롯 내부에 일정 거리 이상 들어온 상태가 지속되면 `OCCUPIED`로 판단한다.
- 차량과의 거리가 기준 이상 멀어진 상태가 지속되면 `EMPTY`로 판단한다.
- Slot 1과 Slot 2가 모두 `OCCUPIED`이면 주차장 상태를 `FULL`로 판단한다.
- 하나 이상의 슬롯이 `EMPTY`이면 주차장 상태를 `AVAILABLE`로 판단한다.

초음파 센서 주의사항:
- HC-SR04 센서를 동시에 측정하지 않는다.
- 초음파 간섭을 방지하기 위해 각 센서는 순차적으로 측정한다.
- 튀는 거리값에 대비해 이동평균 또는 중앙값 필터를 적용한다.

발행 토픽:
- `parking/rpi1/slot`
- `parking/rpi1/distance`
- `parking/rpi1/lot`

### Rpi #2: 바리게이트 제어 노드

역할:
- 입구 초음파 센서로 차량 접근 감지
- RPI1의 주차장 상태(`AVAILABLE`/`FULL`) 수신
- Servo Motor로 게이트 open/close 제어
- 출차 스위치 입력 처리
- 만차 LED 및 출차 이벤트 LED/Buzzer 제어
- MQTT로 게이트 상태와 이벤트 발행

하드웨어:
- Ultrasonic #3: 주차장 입구 차량 접근 감지
- Switch: 출차 요청 버튼
- Servo Motor: 게이트 open/close
- LED: 만차 상태 표시
- LED/Buzzer: 출차 이벤트 상태 표시

제어 시나리오:
- 입구 초음파 센서가 차량 접근을 감지하면 게이트를 `OPEN`한다.
- 게이트가 열린 뒤 일정 시간이 지나면 `CLOSED`로 전환한다.
- 게이트가 열린 상태에서 차량 접근이 새로 감지되면 close timer를 갱신한다.
- 출차 스위치가 눌리면 게이트를 `OPEN`하고 `EXIT_OPEN` 이벤트를 발행한다.
- RPI1에서 `FULL`을 수신하면 만차 LED를 점등한다.
- 만차 상태에서도 차량 접근 시 게이트는 열리며, LED와 Dashboard로 주차 가능 공간이 없음을 안내한다.

구독 토픽:
- `parking/rpi1/lot`

발행 토픽:
- `parking/rpi2/gate`
- `parking/rpi2/event`

### Rpi #3: 중앙 서버 및 Dashboard

역할:
- MQTT Broker 실행
- RPI1/RPI2 메시지 구독
- 슬롯 점유 상태 저장
- 게이트 상태 저장
- 이벤트 로그 저장
- Dashboard에 실시간 상태 표시

구독 토픽:
- `parking/rpi1/slot`
- `parking/rpi1/distance`
- `parking/rpi1/lot`
- `parking/rpi2/gate`
- `parking/rpi2/event`

DB 저장 항목:
- 슬롯별 점유 상태
- 슬롯별 거리값
- 전체 주차장 상태
- 게이트 상태
- 게이트 이벤트
- 수신 시간

---

## 3. Dashboard UI 가이드라인

Dashboard는 실제 주차장 모형의 현재 상태를 빠르게 파악할 수 있도록 구성한다.

표시 항목:
- Slot 1 상태: `EMPTY` / `OCCUPIED`
- Slot 2 상태: `EMPTY` / `OCCUPIED`
- Parking Lot 상태: `AVAILABLE` / `FULL`
- Gate 상태: `OPEN` / `CLOSED`
- 최근 이벤트 로그

권장 레이아웃:
- 좌측: 2개 슬롯과 입구 바리게이트를 보여주는 Top-down 주차장 View
- 우측: 슬롯 상태, 거리값, 전체 주차장 상태, 게이트 상태, 최근 이벤트 로그 패널

시각적 피드백:
- `EMPTY` 슬롯은 비어 있는 상태로 표시한다.
- `OCCUPIED` 슬롯은 차량 또는 점유 색상으로 표시한다.
- `FULL` 상태는 Dashboard와 입구 LED 상태가 일치하도록 강조한다.
- `OPEN`/`CLOSED`에 따라 바리게이트 아이콘 또는 애니메이션을 변경한다.

---

## 4. 물리 모형 제작 기준

- 주차장 모형은 박스 상자로 육면체를 구성한다.
- 입구에는 서보 모터 기반 바리게이트를 배치한다.
- 내부에는 Slot 1, Slot 2를 구분해 배치한다.
- 만차 시 차량이 주차하지 않고 이동할 수 있는 회차 공간을 확보한다.
- 센서는 초음파 간섭을 줄일 수 있도록 측정 방향과 간격을 조정한다.
