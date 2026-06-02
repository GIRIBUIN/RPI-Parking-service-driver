# 개발 스케줄

README 기준의 2개 주차 슬롯, 입구 바리게이트, MQTT/DB/Dashboard 구조에 맞춘 개발 일정이다.

---

## Rpi #1 담당자: 주차 슬롯 점유 판단 노드

### 1주 차: 하드웨어 결선 및 개별 센서 테스트

- [ ] **Task 1.1**: Raspberry Pi OS 설치 및 SSH/VS Code 원격 개발 환경 구축
- [ ] **Task 1.2**: Slot 1용 HC-SR04 초음파 센서 결선 및 개별 거리 측정 테스트
- [ ] **Task 1.3**: Slot 2용 HC-SR04 초음파 센서 결선 및 개별 거리 측정 테스트
- [ ] **Task 1.4**: 두 초음파 센서를 순차 측정하는 로직 구현
- [ ] **Task 1.5**: Slot 1, Slot 2 LED 점등/소등 GPIO 테스트

### 2주 차: 슬롯 상태 판단 및 MQTT 발행

- [ ] **Task 2.1**: 거리 임계값 기반 `EMPTY`/`OCCUPIED` 판단 로직 구현
- [ ] **Task 2.2**: 상태 흔들림 방지를 위한 지속 시간 조건 또는 필터링 적용
- [ ] **Task 2.3**: Slot 1, Slot 2 상태에 따라 LED 점등/소등 매핑
- [ ] **Task 2.4**: `parking/rpi1/slot` 발행 구현
- [ ] **Task 2.5**: `parking/rpi1/distance` 발행 구현
- [ ] **Task 2.6**: 전체 주차장 상태 `AVAILABLE`/`FULL` 판단 후 `parking/rpi1/lot` 발행 구현

### 3주 차: 물리 모형 적용 및 연동 테스트

- [ ] **Task 3.1**: 박스 상자 주차장 모형에 Slot 1, Slot 2 센서와 LED 부착
- [ ] **Task 3.2**: 자동차 모형을 슬롯에 넣고 빼며 `EMPTY`/`OCCUPIED` 전환 테스트
- [ ] **Task 3.3**: Slot 1, Slot 2가 모두 점유되면 `FULL`이 발행되는지 확인
- [ ] **Task 3.4**: 빈 슬롯이 생기면 `AVAILABLE`이 발행되는지 확인
- [ ] **Task 3.5**: RPI3 Dashboard에 슬롯 상태와 거리값이 반영되는지 확인

### 4주 차: 안정화

- [ ] **Task 4.1**: 초음파 센서 노이즈 대응을 위한 이동평균 또는 중앙값 필터 적용
- [ ] **Task 4.2**: MQTT 연결 끊김 시 재연결 로직 점검
- [ ] **Task 4.3**: 장시간 구동 시 슬롯 상태 오판단 여부 확인

---

## Rpi #2 담당자: 입구 바리게이트 제어 노드

### 1주 차: 게이트, 스위치, 입구 센서 테스트

- [x] **Task 1.1**: Raspberry Pi OS 설치 및 원격 개발 환경 구축
- [x] **Task 1.2**: Servo Motor PWM 제어 스크립트 작성 및 `OPEN`/`CLOSED` 각도 튜닝
- [x] **Task 1.3**: 출차 요청 Switch 결선 및 인터럽트 감지 테스트
- [ ] **Task 1.4**: 입구 차량 접근 감지용 HC-SR04 초음파 센서 결선 및 거리 측정 테스트
- [ ] **Task 1.5**: 만차 LED 점등/소등 테스트
- [ ] **Task 1.6**: 출차 이벤트 LED/Buzzer 테스트
- [x] **Task 1.7**: `make` 빌드 성공 확인 및 실행 파일 생성

### 2주 차: 게이트 제어 로직 및 MQTT 연동

- [x] **Task 2.1**: RPI3 MQTT Broker 연결 코드 작성
- [ ] **Task 2.2**: `parking/rpi1/lot` 구독 구현
- [ ] **Task 2.3**: `FULL` 수신 시 만차 LED 표시 로직 구현
- [ ] **Task 2.4**: 입구 차량 접근 감지 시 게이트 `OPEN` 제어 구현
- [ ] **Task 2.5**: 게이트 open 후 timer 만료 시 `CLOSED` 전환 구현
- [ ] **Task 2.6**: 차량 접근이 새로 감지되면 close timer 갱신 구현
- [x] **Task 2.7**: 출차 Switch 입력 시 게이트 `OPEN` 제어 구현
- [x] **Task 2.8**: `parking/rpi2/gate` 발행 구현
- [ ] **Task 2.9**: `parking/rpi2/event`에 `ENTRY_OPEN`, `EXIT_OPEN` 발행 구현

### 3주 차: 게이트 모형 조립 및 통합 테스트

- [ ] **Task 3.1**: 물리적 바리게이트 모형에 Servo Motor와 Switch 고정
- [ ] **Task 3.2**: Switch 디바운싱 처리 추가
- [ ] **Task 3.3**: 차량 접근 -> 게이트 open -> timer close 시나리오 테스트
- [ ] **Task 3.4**: 출차 버튼 -> 게이트 open -> timer close 시나리오 테스트
- [ ] **Task 3.5**: 만차 상태에서도 게이트는 open되고 LED/Dashboard로 안내되는지 확인

### 4주 차: 안정화

- [ ] **Task 4.1**: Servo Motor 과부하 방지를 위한 PWM 신호 제어 점검
- [ ] **Task 4.2**: MQTT 통신 장애 시 게이트 안전 동작 정책 점검
- [ ] **Task 4.3**: 입구 초음파 센서 오감지로 인한 불필요한 open 방지 로직 점검

---

## Rpi #3 담당자: 중앙 서버 및 Dashboard

### 1주 차: 브로커, DB, 백엔드 환경 구축

- [ ] **Task 1.1**: Raspberry Pi OS 설치 및 Eclipse Mosquitto 설치/구동
- [ ] **Task 1.2**: Python 백엔드 프로젝트 초기화
- [ ] **Task 1.3**: SQLite DB 스키마 설계
- [ ] **Task 1.4**: MQTT 수신 데몬 또는 백그라운드 구독 로직 구조 설계

DB 저장 대상:
- 슬롯별 점유 상태
- 슬롯별 거리값
- 전체 주차장 상태
- 게이트 상태
- 게이트 이벤트
- 수신 시간

### 2주 차: MQTT 수신 및 데이터 저장

- [ ] **Task 2.1**: `parking/rpi1/slot` 구독 및 DB 저장 구현
- [ ] **Task 2.2**: `parking/rpi1/distance` 구독 및 DB 저장 구현
- [ ] **Task 2.3**: `parking/rpi1/lot` 구독 및 DB 저장 구현
- [ ] **Task 2.4**: `parking/rpi2/gate` 구독 및 DB 저장 구현
- [ ] **Task 2.5**: `parking/rpi2/event` 구독 및 이벤트 로그 저장 구현
- [ ] **Task 2.6**: Dashboard가 조회할 현재 상태 API 구현

### 3주 차: Dashboard 구현

- [ ] **Task 3.1**: Slot 1, Slot 2 상태 표시 UI 구현
- [ ] **Task 3.2**: Parking Lot 상태 `AVAILABLE`/`FULL` 표시 UI 구현
- [ ] **Task 3.3**: Gate 상태 `OPEN`/`CLOSED` 표시 UI 구현
- [ ] **Task 3.4**: 최근 이벤트 로그 표시 UI 구현
- [ ] **Task 3.5**: 2개 슬롯과 입구 바리게이트를 포함한 Top-down View 구현

### 4주 차: 최종 통합 및 시연 준비

- [ ] **Task 4.1**: RPI1/RPI2/RPI3 전체 MQTT 통신 테스트
- [ ] **Task 4.2**: 정상 입차 시나리오 Dashboard 반영 확인
- [ ] **Task 4.3**: 만차 시나리오 Dashboard 반영 확인
- [ ] **Task 4.4**: 출차 시나리오 Dashboard 반영 확인
- [ ] **Task 4.5**: 장시간 구동 시 DB 저장과 UI 갱신 안정성 확인
