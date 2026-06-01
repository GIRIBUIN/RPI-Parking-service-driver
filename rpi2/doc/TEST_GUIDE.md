# RPI2 바리게이트 제어 노드 — Ubuntu 환경 테스트 가이드

## 개요
VirtualBox Ubuntu 환경에서 RPI2 바리게이트 제어 노드를 **단독 테스트**한다.
모든 테스트는 Ubuntu에서 수행하고, 검증 완료 후 RPI2로 배포한다.
RPI3의 만차 신호는 `mosquitto_pub` CLI로 직접 주입한다.

---

## 사전 준비

### 1. 브레드보드 배선 확인

다음 부품을 브레드보드에 연결하세요:

#### HC-SR04 초음파 센서
```
HC-SR04 핀    RPi 핀       설명
─────────────────────────────────
VCC    →      5V (핀 2)   
GND    →      GND (핀 6)
TRIG   →      GPIO 17 (핀 11) — 직접 연결
ECHO   →      GPIO 27 (핀 13) — 전압분배: 1kΩ 후 GPIO 27, 2kΩ 후 GND
```

**중요: ECHO 핀은 5V 출력이므로 반드시 1kΩ(GPIO 방향) + 2kΩ(GND 방향) 전압분배 저항 사용!**

#### 28BYJ-48 스테퍼 모터 + ULN2003 드라이버

ULN2003 모듈의 4개 신호 핀을 다음과 같이 연결:

```
ULN2003 핀    RPi 물리 핀   BCM GPIO   설명
───────────────────────────────────────
IN1           29            GPIO 5     
IN2           31            GPIO 6     
IN3           33            GPIO 13    
IN4           35            GPIO 19    
VCC (+)       2 or 4        —          5V (필수!)
GND (-)       6,9,14,20등   —          공통 접지
```

**중요: ULN2003은 5V 전원이 필수! 3.3V는 불충분하다.**

28BYJ-48 모터는 ULN2003 모듈에 연결 (케이블 내장).

#### 능동 부저

**2핀 부저인 경우:**
```
부저 핀       RPi 핀       설명
─────────────────────────────────
+ (양극)     GPIO 22 (핀 15)
- (음극)     GND
```

**4핀 부저 모듈인 경우:**
```
부저 핀 표시      RPi 핀          설명
──────────────────────────────────────────
S / SIG / I/O    GPIO 22 (핀 15)  신호 핀 ← 이 핀을 GPIO 22에 연결
VCC / +          5V 또는 3.3V     전원
GND / -          GND              접지
NC               연결 안 함        No Connect
```

**필수: 능동 부저(Active Buzzer)만 사용. 수동 부저는 소리 안 남!**

#### 출차 버튼 (택트 스위치)
```
버튼 핀       RPi 핀          설명
────────────────────────────────────
한쪽 끝      3.3V (핀 1) → 10kΩ 저항 → GPIO 23 (핀 16)
다른쪽 끝    GND

※ 10kΩ 외부 풀업 저항 필수! (sysfs GPIO는 내부 풀업 불가)
```

#### LED × 2
```
입차 감지 LED:
  양극 → 220Ω → GPIO 24 (핀 18)
  음극 → GND

만차 상태 LED:
  양극 → 220Ω → GPIO 25 (핀 22)
  음극 → GND
```

#### 최종 배선 요약표
| 컴포넌트 | RPi 물리 핀 | BCM GPIO | 메모 |
|---|---|---|---|
| HC-SR04 TRIG | 11 | 17 | 직접 연결 |
| HC-SR04 ECHO | 13 | 27 | 1kΩ+2kΩ 전압분배 |
| **ULN2003 IN1** | **29** | **5** | **스테퍼 드라이버** |
| **ULN2003 IN2** | **31** | **6** | **스테퍼 드라이버** |
| **ULN2003 IN3** | **33** | **13** | **스테퍼 드라이버** |
| **ULN2003 IN4** | **35** | **19** | **스테퍼 드라이버** |
| 부저 | 15 | 22 | 능동 부저만 |
| 버튼 | 16 | 23 | 10kΩ 풀업 필수 |
| 입차 LED | 18 | 24 | 220Ω 직렬 |
| 만차 LED | 22 | 25 | 220Ω 직렬 |

---

### 2. 소프트웨어 설정 (Ubuntu 환경)

#### 의존성 설치
```bash
sudo apt update
sudo apt install -y libmosquitto-dev mosquitto mosquitto-clients
```

#### 코드 빌드 (Ubuntu에서)
```bash
cd ~/RPI_parking_service/rpi2   # 프로젝트 경로로 이동
make clean
make
```

빌드 결과: `gate_node` x86_64 바이너리 생성
- 이 바이너리는 **Ubuntu에서만 실행 가능** (RPI에서는 실행 불가)
- RPI에 배포하려면: `make scp-src` 후 RPI에서 다시 빌드

---

## 테스트 실행 (Ubuntu 환경)

### 터미널 A: MQTT 브로커 시작

```bash
# 1. MQTT 브로커 시작 (백그라운드)
mosquitto -d

# 또는 포그라운드에서 실행 (로그 확인용)
mosquitto
```

### 터미널 B: gate_node 실행

```bash
cd ~/RPI_parking_service/rpi2
./gate_node
```

성공 시 다음 로그 출력:
```
RPI2 바리게이트 제어 노드 시작
Switch 모니터링 시작
MQTT 연결 시도
초음파 센서 초기화 완료
부저 초기화 완료
MQTT 연결 성공
구독: parking/rpi3/capacity
게이트 초기 상태: CLOSED

[IDLE] 거리: 120.3 cm        (← 3초마다 반복)
```

**주의: 터미널 C에서 아래 테스트 시나리오 실행**

---

## 테스트 시나리오

### ⚠️ 주의사항
RPI2는 **MQTT 수신만** 수행 (`parking/rpi3/capacity` 구독)
발행 기능 없음 → 게이트 상태/이벤트는 로그로만 확인

**GPIO 물리 테스트** (센서, LED, 부저, 버튼): 실제 RPI 하드웨어 필요
- Ubuntu의 sysfs GPIO는 실제 하드웨어 없음
- Ubuntu에서 로그 확인 후 RPI로 배포 권장

### 시나리오 1: 입차 감지 및 자동 닫힘

**터미널 B(gate_node) 로그 확인:**
```bash
[IDLE] 거리: 45.2 cm         (← 센서 감지 시)
[45.2 cm] 입차 차량 감지 — 게이트 OPEN, LED ON, 부저 ON
```

**실제 RPI에서 물리 테스트:**
1. 손을 초음파 센서 앞에 **50cm 이내로 가져가기**
   - 예상 로그: `[XX.X cm] 입차 차량 감지 — 게이트 OPEN, LED ON, 부저 ON`
   - 실제: 게이트 열림, 입차 LED 켜짐, 부저 울림

2. 손을 **센서에서 치우기**
   - 예상 로그: `[XX.X cm] 차량 사라짐 — 5초 타이머 시작`
   - 예상: 부저 꺼짐 (즉시), 5초 대기

3. **5초 경과 후**
   - 예상 로그: `5초 경과 — 게이트 닫음, LED OFF`
   - 실제: 게이트 닫힘, 입차 LED 꺼짐

---

### 시나리오 2: 출차 요청

**실제 RPI 물리 테스트:**
1. **버튼 누르기** (GPIO 23)
   - 예상 로그: `출차 요청 — 게이트 OPEN, LED ON, 부저 ON, 10초 타이머 시작`
   - 실제: 게이트 열림, LED 켜짐, 부저 울림

2. **센서 앞에 손 가져가기** (차량 감지)
   - 예상 로그: `[XX.X cm] 출차 차량 감지`
   - 실제: 게이트 유지

3. **손 치우기** (차량 빠져나감)
   - 예상 로그: `[XX.X cm] 차량 빠져나감 — 출차 완료, LED OFF, 부저 OFF, 게이트 닫음`
   - 실제: 게이트 닫힘, LED 꺼짐

---

### 시나리오 3: 출차 타임아웃

**실제 RPI 물리 테스트:**
1. **버튼 누르기** (GPIO 23)
   - 예상: 게이트 열림, LED 켜짐, 부저 울림

2. **10초 동안 아무것도 하지 않기**
   - 예상 로그: `10초 경과 (차량 미감지) — LED OFF, 부저 OFF, 게이트 닫음 (타임아웃)`
   - 실제: 10초 후 자동으로 게이트 닫힘, LED 꺼짐, 부저 꺼짐

---

### 시나리오 4: 만차 신호 (MQTT로 주입)

**터미널 C에서:**
```bash
# 만차 신호 발행
mosquitto_pub -h 127.0.0.1 -p 1883 \
  -t "parking/rpi3/capacity" \
  -m "FULL" \
  -q 1 -r

# 또는 한 줄로
mosquitto_pub -h 127.0.0.1 -t "parking/rpi3/capacity" -m "FULL" -q 1 -r
```

**예상:**
- 만차 LED (GPIO 25) 켜짐

**해제하기:**
```bash
mosquitto_pub -h 127.0.0.1 -t "parking/rpi3/capacity" -m "AVAILABLE" -q 1 -r
```

**예상:**
- 만차 LED (GPIO 25) 꺼짐

---

## 문제 해결 (Ubuntu 환경)

### 1. gate_node 실행 오류

**오류: Permission denied**
```bash
# 권한 부족 시
chmod +x gate_node
./gate_node
```

**오류: libmosquitto.so.1 not found**
```bash
# 라이브러리 재설치
sudo apt install -y libmosquitto-dev
```

### 2. MQTT 토픽이 보이지 않음

**가능한 원인:**
- mosquitto 브로커가 실행 중이 아님
- MQTT_HOST가 잘못 설정됨 (현재: `127.0.0.1`)

**확인 방법:**

```bash
# MQTT 브로커 상태 확인
sudo systemctl status mosquitto
ps aux | grep mosquitto

# 로그 확인
sudo tail -f /var/log/mosquitto/mosquitto.log

# 수동 발행 테스트
mosquitto_pub -h 127.0.0.1 -t "test" -m "hello"

# 구독해서 확인
mosquitto_sub -h 127.0.0.1 -t "#"
```

---

## Ubuntu 테스트 체크리스트

- [ ] `make clean && make` 빌드 성공
- [ ] `mosquitto -d` 브로커 실행
- [ ] `./gate_node` 실행, MQTT 연결 로그 출력
  ```
  MQTT 연결 성공
  구독: parking/rpi3/capacity
  ```
- [ ] `[IDLE] 거리: XXX.X cm` 로그 3초마다 출력
- [ ] 터미널 B(gate_node) 로그에서 상태 전환 메시지 확인
- [ ] 만차 신호 MQTT 테스트:
  ```bash
  mosquitto_pub -h 127.0.0.1 -t "parking/rpi3/capacity" -m "FULL" -q 1 -r
  # → 터미널 C에서 "만차 신호 수신 → 만차 LED ON" 로그 확인
  ```

---

## RPI2로 배포

Ubuntu에서 테스트 완료 후 실제 RPI2에 배포:

### 방법 1: 소스 전송 (권장)
```bash
# Ubuntu에서
make scp-src              # 소스 파일 전송

# RPI2 SSH 접속 후
cd ~/rpi2
sudo apt install -y libmosquitto-dev
make                      # ARM 바이너리 빌드
./gate_node               # 실행
```

### 방법 2: 바이너리 전송 (cross-compile 환경만)
```bash
# arm-linux-gnueabihf-gcc 등으로 빌드한 경우만
make scp
# → pi@10.10.10.12:~/ 로 전송
```

### RPI2에서 MQTT 호스트 변경
만약 실제 MQTT 브로커(192.168.1.100)를 사용한다면:

```bash
# config.h 수정
#define MQTT_HOST "192.168.1.100"

make clean && make
./gate_node
```
