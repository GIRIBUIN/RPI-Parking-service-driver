# 제작 가이드라인

---

###  1. 시스템 아키텍처 및 기술 스택 (Tech Stack)

기획서에 명시된 3대의 라즈베리파이(Raspberry Pi)를 효율적으로 연결하고 제어하기 위해 다음과 같은 스택을 확정합니다.

* **하드웨어 및 OS**: Raspberry Pi 3대 (Raspberry Pi OS)
* **디바이스 제어 (Rpi #1, #2)**: Python 3 (`RPi.GPIO` 또는 `gpiozero` 라이브러리 사용)
* **통신 프로토콜**: MQTT (실시간 비동기 통신에 최적화)
    * **Broker**: Eclipse Mosquitto (Rpi #3에 설치)
    * **Client**: Python `paho-mqtt` 라이브러리
* **백엔드 및 DB (Rpi #3)**: Python FastAPI (가볍고 빠른 API 서버) + SQLite
* **프론트엔드 (Digital Twin Dashboard)**: React.js + Tailwind CSS + 2D Canvas (HTML5)
    * `mqtt.js`를 사용하여 브라우저에서 직접 MQTT 토픽을 구독해 지연 없는 실시간 렌더링 구현.

---

###  2. 디바이스별 구현 가이드라인

####  Rpi #1: 주차 공간 센싱 및 경고 노드
**역할**: 센싱, 상태 판단, 경고 발생
* **하드웨어 연동**:
    * **Ultrasonic #1 (정면/입구)**: 주차 공간 차량 접근 감지.
    * **Ultrasonic #2 (후면)**: 주차 완료 및 후방 충돌 감지.
    * **Ultrasonic #3 (우측/측면)**: 측면 충돌 감지.
* **[CRITICAL] 초음파 센서 제어 주의사항**: HC-SR04 센서 3개를 동시에 측정하면 초음파 간섭이 발생하므로, 반드시 코드 상에서 **순차적(Sequential)으로 측정**하도록 딜레이 로직을 구현해야 합니다.
* **상태 판별 로직**: 
    * 센서값을 바탕으로 상태를 `EMPTY`, `APPROACHING`, `PARKING`, `PARKED`로 전환하여 MQTT로 퍼블리싱.
    * `PARKED` 상태 판단 시 `parking/control/gate` 토픽으로 `CLOSE` 명령을 발행하여 Rpi #2의 게이트 제어를 트리거함.
* **경고 시스템 (위험도에 따른 제어)**:
    * `SAFE` (안전 거리): LED 소등, Buzzer 무음.
    * `WARNING` (주의 거리): LED 깜빡임, Buzzer 간헐적 beep.
    * `DANGER` (충돌 위험): LED 점등 유지, Buzzer 연속 경고음.

####  Rpi #2: 바리게이트 제어 노드
**역할**: 게이트 모터 제어, 출차 요청 이벤트 발생
* **하드웨어 연동**: Servo Motor (게이트 제어), Switch (출차 버튼), LED (상태 표시).
* **제어 시나리오**:
    * 기본 상태는 `OPEN` 유지.
    * `parking/control/gate` 토픽에서 `CLOSE` 명령을 수신하면 서보 모터를 구동하여 `CLOSED`로 전환.
    * 사용자가 Switch(버튼)를 누르면 출차 요청 이벤트를 `parking/rpi2/event` 토픽으로 발행하며 모터가 `OPEN` 상태로 전환.
    * 게이트 상태(`OPEN`/`CLOSED`) 변경 시 `parking/rpi2/gate` 토픽으로 상태를 MQTT 퍼블리싱.

####  Rpi #3: 중앙 서버 및 Digital Twin
**역할**: MQTT 통신 중계, DB 데이터 저장, Dashboard 모니터링 제공. 센서/액추에이터는 연결되지 않습니다.
* **MQTT 구독**: Rpi #1의 주차 상태, 거리, 위험도 정보와 Rpi #2의 게이트 상태(`parking/rpi2/gate`), 출차 이벤트(`parking/rpi2/event`)를 구독.
* **DB 저장**: 구독한 모든 데이터(거리 측정값, 주차 상태, 위험도, 게이트 상태, 이벤트)를 타임스탐프와 함께 DB에 적재.

---

###  3. 2D Digital Twin Dashboard UI/UX 가이드라인 (PM 지시사항)

Rpi #3에서 호스팅될 웹 대시보드는 50x30cm의 물리적 모형을 화면에 그대로 옮겨놓은 듯한 "직관성"이 생명입니다.

* **레이아웃 구성**:
    * **좌측 (70%) - 2D 주차장 View**: 위에서 아래를 내려다보는 Top-down 뷰. 센서값(거리)을 역산하여 15x8cm 크기의 자동차 컴포넌트 위치를 캔버스 상에서 실시간으로 이동시킵니다.
    * **우측 (30%) - Status Panel**: 현재 주차 상태(EMPTY/APPROACHING 등), 각 센서별 실시간 거리(cm), 현재 위험도 상태(SAFE/WARNING/DANGER), 게이트 상태(OPEN/CLOSED)를 텍스트와 게이지 바 형태로 보여줍니다.
* **동기화 및 시각적 피드백**:
    * 실제 LED/Buzzer 상태와 동일하게 UI 상의 주차장 테두리 색상을 변경합니다 (SAFE: 초록, WARNING: 노랑 + 점멸 효과, DANGER: 빨강 + 경고 팝업).
    * 좌측 하단에 게이트 아이콘을 배치하고 OPEN/CLOSED 상태에 따라 애니메이션을 적용하세요.

---

팀원 여러분, 위 가이드라인을 바탕으로 각자의 파트(Rpi 1, 2, 3) 구성을 시작해 주시기 바랍니다.