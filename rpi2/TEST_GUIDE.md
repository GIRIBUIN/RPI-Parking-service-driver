# RPI2 바리게이트 제어 노드 — 크로스 컴파일 배포 가이드

## 개요

RPI2 바리게이트 제어 노드는 **Ubuntu 환경에서 ARM용 크로스 컴파일**로 커널 모듈을 생성하고,
**RPI2로 전송**하여 실행하는 방식으로 동작한다.

- **빌드 환경:** Ubuntu (x86_64)
- **크로스 컴파일러:** `arm-linux-gnueabihf-gcc` (32-bit ARMv7)
- **배포 방식:** `scp gate_node.ko` → RPI2 → `sudo insmod gate_node.ko`
- **구조:** 커널 모듈 (`gate_node.ko`) + MQTT 브릿지 (`mqtt_bridge`)

---

## Ubuntu 환경 설정

### 1. 크로스 컴파일러 설치

```bash
sudo apt update
sudo apt install -y gcc-arm-linux-gnueabihf

# 확인
arm-linux-gnueabihf-gcc --version
```

### 2. RPI2 커널 헤더 준비

#### 2.1. RPI2에 커널 헤더 설치 (SSH 접속)

```bash
ssh pi@10.10.10.12
sudo apt install -y raspberrypi-kernel-headers
uname -r  # 버전 확인 (예: 6.1.21-v7+)
exit
```

#### 2.2. Ubuntu로 커널 헤더 복사 (최초 1회)

```bash
# RPI_parking_service/rpi2 디렉토리에서:
make get-headers
# → ~/rpi-kernel-headers/ 생성 (필요한 커널 헤더 다운로드)
```

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

※ 10kΩ 외부 풀업 저항 필수!
```

#### LED × 3
```
입차 LED:
  양극 → 220Ω → GPIO 24 (핀 18)
  음극 → GND

출차 LED:
  양극 → 220Ω → GPIO 26 (핀 37)
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
| 출차 LED | 37 | 26 | 220Ω 직렬 |
| 만차 LED | 22 | 25 | 220Ω 직렬 |

---

## 빌드 및 배포

### Ubuntu에서 크로스 컴파일

```bash
cd ~/RPI_parking_service/rpi2

# 빌드
make clean
make

# 결과 확인
file gate_node.ko
# → gate_node.ko: ELF 32-bit LSB relocatable, ARM, EABI5 version 1 (SYSV)...
```

### RPI2로 모듈 전송 및 설치

```bash
# Ubuntu에서:
make scp
# → gate_node.ko를 pi@10.10.10.12:~/ 로 전송

# RPI2에서 (SSH 접속):
sudo insmod gate_node.ko

# 확인
lsmod | grep gate_node
# → gate_node               12288  0

# 로그 확인
sudo dmesg | tail -10
```

### MQTT 브릿지 설치

```bash
# Ubuntu에서 mqtt_bridge 소스 전송:
make scp-src

# RPI2에서 (SSH 접속):
cd ~/rpi2
sudo apt install -y libmosquitto-dev mosquitto

# mqtt_bridge 빌드
gcc -Wall -pthread -o mqtt_bridge mqtt_bridge.c -lmosquitto

# MQTT 브로커 시작 (백그라운드)
mosquitto -d

# MQTT 브릿지 실행 (새 터미널)
./mqtt_bridge
```

예상 출력:
```
MQTT Bridge Starting
[DEVICE] Opened /dev/gate_node
[MQTT] Connecting to 127.0.0.1:1883
[MQTT] Connected successfully
[MQTT] Subscribed: parking/rpi3/capacity
[MQTT] Connected and listening
```

---

## 테스트 시나리오

### ⚠️ 주의사항

RPI2는 **MQTT 수신만** 수행 (`parking/rpi3/capacity` 구독).
발행 기능 없음 → 게이트 상태/이벤트는 커널 로그로 확인.

**GPIO 물리 테스트** (센서, LED, 부저, 버튼): 실제 RPI 하드웨어 필수.

### 시나리오 1: 입차 감지 및 자동 닫힘

**실제 RPI에서 물리 테스트:**

1. **손을 초음파 센서 앞에 50cm 이내로 가져가기**
   - 예상 커널 로그: `[XX cm] 입차 차량 감지 — 게이트 OPEN`
   - 실제: 게이트 열림, **입차 LED (GPIO 24) 깜빡임**, 부저 깜빡임

2. **손을 센서에서 치우기**
   - 예상 로그: `[XX cm] 차량 사라짐 — 5초 타이머 시작`
   - 실제: 부저/LED 깜빡임 유지, 5초 대기

3. **5초 경과 후**
   - 예상 로그: `5초 경과 — 게이트 닫음`
   - 실제: 게이트 닫힘, 입차 LED 꺼짐, 부저 꺼짐

---

### 시나리오 2: 출차 요청

**실제 RPI 물리 테스트:**

1. **버튼 누르기** (GPIO 23)
   - 예상 로그: `[SWITCH] 출차 요청 — 게이트 OPEN`
   - 실제: 게이트 열림, **출차 LED (GPIO 26) 깜빡임**, 부저 깜빡임

2. **센서 앞에 손 가져가기** (차량 감지)
   - 예상 로그: `[XX cm] 출차 차량 감지`
   - 실제: 게이트 유지, 출차 LED/부저 깜빡임 유지

3. **손 치우기** (차량 빠져나감)
   - 예상 로그: `[XX cm] 차량 빠져나감 — 출차 완료, 게이트 닫음`
   - 실제: 게이트 닫힘, 출차 LED 꺼짐, 부저 꺼짐

---

### 시나리오 3: 출차 타임아웃

**실제 RPI 물리 테스트:**

1. **버튼 누르기** (GPIO 23)
   - 예상: 게이트 열림, **출차 LED (GPIO 26) 깜빡임**, 부저 깜빡임

2. **10초 동안 아무것도 하지 않기**
   - 예상 로그: `10초 경과 (차량 미감지) — 게이트 닫음 (타임아웃)`
   - 실제: 10초 후 자동으로 게이트 닫힘, 출차 LED 꺼짐, 부저 꺼짐

---

### 시나리오 4: 만차 신호 (MQTT로 주입)

**RPI2의 다른 터미널에서:**

```bash
# 만차 신호 발행
mosquitto_pub -h 127.0.0.1 -t "parking/rpi3/capacity" -m "FULL" -q 1 -r
```

**예상:**
- 커널 로그: `Capacity FULL`
- 만차 LED (GPIO 25) 켜짐

**해제하기:**

```bash
mosquitto_pub -h 127.0.0.1 -t "parking/rpi3/capacity" -m "AVAILABLE" -q 1 -r
```

**예상:**
- 커널 로그: `Capacity AVAILABLE`
- 만차 LED (GPIO 25) 꺼짐

---

## 커널 로그 모니터링

```bash
# 실시간 커널 메시지 모니터링
sudo dmesg -w

# 또는 마지막 20줄만 보기
sudo dmesg | tail -20

# 모듈 적재 관련 로그만 필터링
sudo dmesg | grep -i gate
```

---

## 문제 해결

### 1. 크로스 컴파일 실패

**오류: make: *** /lib/modules/.../build: 그런 파일이나 디렉터리가 없습니다**

```bash
# RPI 커널 헤더를 다시 다운로드
make clean
make get-headers
make
```

**오류: arm-linux-gnueabihf-gcc: 명령을 찾을 수 없음**

```bash
sudo apt install -y gcc-arm-linux-gnueabihf
```

### 2. `sudo insmod gate_node.ko` 실패

**오류: insmod: ERROR: could not load module gate_node.ko**

```bash
# 로그 확인
sudo dmesg | tail -20

# 흔한 원인:
# 1) 커널 버전 불일치 → RPI에서 sudo apt install raspberrypi-kernel-headers 재실행
# 2) 모듈 서명 문제 (Secure Boot) → Raspberry Pi는 보통 없음
```

### 3. `/dev/gate_node` 파일 없음

```bash
# 확인
ls -l /dev/gate_node

# 없으면 모듈 미적재 확인:
lsmod | grep gate_node

# 모듈 재적재:
sudo rmmod gate_node 2>/dev/null || true
sudo insmod gate_node.ko
```

### 4. MQTT 신호 수신 안 됨

```bash
# MQTT 브로커 상태 확인
ps aux | grep mosquitto

# 브로커 시작:
mosquitto -d

# mqtt_bridge 실행:
./mqtt_bridge

# 테스트 발행:
mosquitto_pub -h 127.0.0.1 -t "test" -m "hello"
mosquitto_sub -h 127.0.0.1 -t "#"
```

---

## 정상 종료

```bash
# 1) mqtt_bridge 종료 (Ctrl+C)
# 2) MQTT 브로커 종료 (선택사항)
sudo pkill mosquitto

# 3) 커널 모듈 언로드
sudo rmmod gate_node

# 확인
lsmod | grep gate_node
# → 아무것도 출력 안 됨 (정상)
```

---

## 빌드 속도 팁

**크로스 컴파일이 느린 경우:**

```bash
# 다중 코어 사용 (-j 플래그)
make -j$(nproc)
```

---

## 참고: Makefile 타겟

| 명령 | 설명 |
|---|---|
| `make` | 크로스 컴파일로 `gate_node.ko` 생성 |
| `make module` | 동일 (모듈만 빌드) |
| `make bridge` | `mqtt_bridge` 로컬 빌드 (테스트용) |
| `make get-headers` | RPI 커널 헤더 다운로드 (최초 1회) |
| `make scp` | `gate_node.ko`를 RPI로 전송 |
| `make scp-src` | `mqtt_bridge.c` 소스를 RPI로 전송 |
| `make clean` | 빌드 산출물 정리 |
| `make help` | 도움말 |

---

## Ubuntu 테스트 체크리스트

- [ ] `sudo apt install gcc-arm-linux-gnueabihf` 완료
- [ ] RPI에서 `sudo apt install raspberrypi-kernel-headers` 완료
- [ ] `make get-headers` 실행 → `~/rpi-kernel-headers/` 생성 확인
- [ ] `make` 빌드 성공 → `gate_node.ko` 생성
- [ ] `file gate_node.ko` → "ELF 32-bit" 확인
- [ ] `make scp` → RPI2로 전송
- [ ] RPI2: `sudo insmod gate_node.ko` → `lsmod | grep gate_node` 확인
- [ ] RPI2: `sudo dmesg | tail -5` → 초기화 로그 확인
- [ ] RPI2: `mosquitto -d` + `./mqtt_bridge` 실행
- [ ] 센서 테스트: 손을 초음파 앞에 가져가기 → **입차 LED (GPIO 24) 깜빡임**, 부저 울림
- [ ] 버튼 테스트: GPIO 23 누르기 → **출차 LED (GPIO 26) 깜빡임**, 부저 울림
- [ ] MQTT 만차 신호:
  ```bash
  mosquitto_pub -t "parking/rpi3/capacity" -m "FULL" -q 1 -r
  # → 커널 로그: "Capacity FULL"
  # → 만차 LED 켜짐
  ```
- [ ] 모듈 언로드: `sudo rmmod gate_node` 성공
