# RPI2 바리게이트 제어 노드 구현 계획 (C언어, wiringPi 미사용)

## Context
Digital Twin 스마트 주차 시스템 팀 프로젝트에서 RPI2(바리게이트 제어 노드)를 담당.
개발 언어가 C로 확인됨 (README.md에 C 명시).
외부 라이브러리 의존성 최소화를 위해 **wiringPi를 사용하지 않고**,
표준 커널 인터페이스(`/sys/class/gpio/`, `/sys/class/pwm/`)로 GPIO와 PWM을 직접 제어한다.

---

## 파일 구조

```
rpi2/
├── README.md                (기존)
├── PLAN.md                  (이 파일)
├── Makefile                 (빌드 설정)
├── config.h                 (핀 번호, PWM, MQTT 상수)
├── gate_controller.h        (서보 모터 + LED 인터페이스)
├── gate_controller.c        (서보 모터 + LED 구현, /sys/class/pwm/ 사용)
├── switch_handler.h         (Switch 인터럽트 인터페이스)
├── switch_handler.c         (poll(2)로 GPIO edge 감지)
├── mqtt_client.h            (MQTT 인터페이스)
├── mqtt_client.c            (paho-mqtt C 클라이언트 구현)
└── main.c                   (진입점, 전체 루프)
```

---

## 기술 스택 (C언어, wiringPi 미사용)

| 구분 | 방식 | 설명 |
|------|------|------|
| GPIO 제어 | `/sys/class/gpio/` | export, direction, value, edge 파일 직접 접근 |
| PWM 제어 | `/sys/class/pwm/` | period, duty_cycle, enable 파일 직접 접근 |
| MQTT | Eclipse Paho C | `libpaho-mqtt3c-dev` |
| 스레드 / 뮤텍스 | POSIX pthreads | `libpthread` (기본 내장) |
| 이벤트 감지 | poll(2) | GPIO edge FALLING 감지 |

설치 명령:
```bash
sudo apt-get install libpaho-mqtt3-dev
```

---

## GPIO 핀 할당 (BCM)

| 핀 | BCM | 물리 핀 | 역할 |
|----|-----|---------|------|
| 서보 모터 PWM | 18 | 12 | BCM 18은 haredware PWM0을 지원하므로 /sys/class/pwm/pwmchip0/pwm0 사용 |
| LED | 24 | 18 | GPIO 디지털 출력, `/sys/class/gpio/gpio24/` 사용 |
| Switch | 23 | 16 | GPIO 디지털 입력 (내부 풀업), `/sys/class/gpio/gpio23/` 사용, FALLING edge 감지 |

---

## 서보 PWM 파라미터 (나노초 단위)

- **주기**: 50Hz (20ms = 20,000,000ns)
- **OPEN (0도)**: 0.5ms = 500,000ns
- **CLOSED (~180도)**: 2.4ms = 2,400,000ns
- **이동 대기**: 0.5초 후 duty_cycle = 0으로 설정 → 모터 과부하 방지

### 파일 경로
```
/sys/class/pwm/pwmchip0/
├── export
├── pwm0/
│   ├── period          (20000000)
│   ├── duty_cycle      (500000 또는 2400000 또는 0)
│   └── enable          (1 활성화, 0 비활성화)
└── unexport
```

---

## MQTT 토픽

| 방향 | 토픽 | 내용 |
|------|------|------|
| 구독 | `parking/control/gate` | OPEN / CLOSE 명령 |
| 발행 | `parking/rpi2/gate` | OPEN / CLOSED 상태 (retain=1) |
| 발행 | `parking/rpi2/event` | exit_requested 이벤트 (JSON) |

---

## 핵심 구현 사항

### gate_controller.c
- **GPIO 제어**: `/sys/class/gpio/gpio24/` (LED)
- **PWM 제어**: `/sys/class/pwm/pwmchip0/pwm0/` (서보 모터, BCM 18)
- `gate_controller_init()`: GPIO/PWM 초기화
- `gate_open()`, `gate_close()`: 각도 전환 후 PWM 신호 차단
- `gate_get_state()`: 현재 상태 반환
- `pthread_mutex_t gate_mutex`: MQTT/Switch 동시 호출 보호
- `gate_controller_cleanup()`: PWM/GPIO unexport

### switch_handler.c
- **GPIO edge 감지**: `/sys/class/gpio/gpio23/` (FALLING edge)
- **poll(2)**: GPIO value 파일의 edge 변화 감지 (timeout 500ms)
- `switch_handler_init(callback)`: GPIO export, edge 설정, 모니터링 스레드 시작
- **스레드**: 메인 루프 차단하지 않고 백그라운드에서 edge 감지
- **디바운싱**: `get_time_ms()` 기반 300ms 소프트웨어 필터

### mqtt_client.c
- Eclipse Paho C (동기 클라이언트)
- `mqtt_init(callback)`: 클라이언트 생성, 콜백 등록
- `mqtt_connect()`: 브로커 연결, 자동 재연결 설정
- `mqtt_publish_gate_state()`: QoS 1, retain=1
- `mqtt_publish_event()`: JSON 페이로드, QoS 1
- `mqtt_is_connected()`: 뮤텍스로 보호된 연결 상태 조회

### main.c
- `signal(SIGINT/SIGTERM, shutdown_handler)`: 정상 종료 보장
- 초기화: `gate_controller_init()` → `switch_handler_init()` → `mqtt_init()` → `mqtt_connect()`
- **Failsafe**: 10초 MQTT 미연결 시 `gate_open()` 강제 호출
- 메인 루프: 1초마다 failsafe 확인

---

## 4주차 개발 일정

| 주차 | 핵심 작업 | 완료 기준 |
|------|-----------|-----------|
| 1주차 | GPIO/PWM 수동 테스트 + 빌드 | `make` 빌드 성공, 서보/LED/Switch 수동 제어 확인 |
| 2주차 | MQTT 연결 + 전체 시나리오 통합 | `mosquitto_pub` CLOSE 발행 시 게이트 닫힘, Switch 클릭 → 열림+이벤트 발행 |
| 3주차 | 물리 모형 고정, 디바운싱 처리, RPI1 통합 | 버튼 연속 클릭 시 이벤트 중복 없음, 전체 사이클 성공 |
| 4주차 | 과부하 방지, Failsafe 검증 | 브로커 종료 시 게이트 OPEN 유지, 장시간 안정성 확인 |

---

## 개발 시 주의사항

### GPIO 권한
```bash
# /sys/class/gpio/ 접근 권한 확인
ls -la /sys/class/gpio/

# RPi에서 root 권한 필요 (또는 gpio 그룹 추가)
sudo usermod -a -G gpio $USER
```

### PWM 설정
BCM 18은 하드웨어 PWM0을 지원합니다. `/sys/class/pwm/pwmchip0/`를 사용합니다.
```bash
# 설정 확인
cat /sys/class/pwm/pwmchip0/npwm  # 0이면 pwm0 사용 가능
```

### Switch 감지
poll(2)은 GPIO edge 감지에 최적화되어 있습니다.
300ms 소프트웨어 디바운싱으로 바운싱 노이즈를 필터링합니다.

---

## 검증 방법

```bash
# 1. 빌드
make

# 2. MQTT 토픽 모니터링
mosquitto_sub -h <RPI3_IP> -t "parking/rpi2/#" -v

# 3. CLOSE 명령 발행 (다른 터미널)
mosquitto_pub -h <RPI3_IP> -t "parking/control/gate" -m "CLOSE"

# 4. 게이트 상태 확인 (콘솔에 "게이트 닫힘" 출력)

# 5. Switch 버튼 클릭 (GPIO 23을 물리적으로 누름)
# → 콘솔에 "출차 요청" 출력
# → MQTT에 exit_requested 이벤트 발행됨

# 6. Failsafe 테스트
sudo systemctl stop mosquitto
# → 10초 후 콘솔에 Failsafe 메시지 및 게이트 OPEN 확인
sudo systemctl start mosquitto
```

---

## 참고: wiringPi를 사용하지 않는 이유

1. **외부 의존성 최소화**: 표준 커널 인터페이스만 사용
2. **크로스 플랫폼**: Raspberry Pi뿐만 아니라 다른 Linux 임베디드 시스템 호환
3. **유지보수성**: wiringPi 개발 중단 위험에서 자유로움
4. **성능**: 커널 드라이버 직접 사용으로 오버헤드 감소
