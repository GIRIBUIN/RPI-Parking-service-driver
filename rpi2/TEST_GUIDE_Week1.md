# RPI2 1주차 테스트 가이드

## 환경 구축 (라즈베리파이 키트 + Breadboard)

### 1단계: 라즈베리파이 준비

```bash
# 라즈베리파이에 SSH 접속
ssh pi@<라즈베리파이_IP>

# 비밀번호: raspberry (기본값) 또는 설정한 비밀번호

# 시스템 업데이트
sudo apt-get update && sudo apt-get upgrade -y

# 필수 패키지 설치
sudo apt-get install -y build-essential git libpaho-mqtt3-dev

# GPIO 접근 권한 설정
sudo usermod -a -G gpio pi

# 로그아웃 후 다시 로그인 (권한 적용)
exit
ssh pi@<라즈베리파이_IP>
```

### 2단계: Breadboard 배선 (핀 연결)

라즈베리파이 GPIO 핀 배치:
```
물리 핀 번호            BCM 번호
     3V3 (1)            
      5V (2)            
  GPIO2 (3) - SDA        BCM 2
  GPIO3 (5) - SCL        BCM 3
  
 GPIO4 (7)               BCM 4
  GND (9)                
GPIO17 (11)              BCM 17
GPIO27 (13)              BCM 27
  GND (14)               
GPIO22 (15)              BCM 22
GPIO23 (16) ← SWITCH     BCM 23
  GND (18)               
GPIO24 (18) ← LED        BCM 24  
GPIO25 (22)              BCM 25
  GND (25)               
GPIO18 (12) ← SERVO_PWM  BCM 18
```

### 3단계: Breadboard 배선 (RPI2 회로)

#### 서보 모터 (BCM 18 - GPIO 12번 핀)
```
라즈베리파이 GPIO 12 (BCM 18)
         ↓
    [Breadboard]
         ↓
    서보 모터 신호(황색) → GPIO 12 (핀 번호 12)
    서보 모터 GND(검정)  → GND (핀 번호 9 또는 25)
    서보 모터 VCC(빨강)  → 외부 5V 전원 (라즈베리파이 5V 직결 금지!)
    
⚠️ 중요: 서보 모터는 라즈베리파이의 3.3V로는 작동하지 않음
         외부 USB 전원 또는 배터리 팩의 5V를 사용해야 함
         단, GND는 라즈베리파이와 공유
```

#### LED (BCM 24 - GPIO 18번 핀)
```
라즈베리파이 GPIO 18 (BCM 24)
         ↓
    220Ω 저항 (필수)
         ↓
      LED 양극(+, 긴 다리)
         ↓
   [Breadboard]
         ↓
      LED 음극(-, 짧은 다리)
         ↓
      GND (핀 번호 9 또는 25)

⚠️ 중요: 저항 없으면 GPIO 핀 손상 위험
```

#### Switch 버튼 (BCM 23 - GPIO 16번 핀)
```
라즈베리파이 GPIO 16 (BCM 23)
         ↓
   [Breadboard]
         ↓
   Switch 한쪽 끝 ← GPIO 16 (핀 번호 16)
         ↓
   Switch 다른 끝 → GND (핀 번호 9 또는 25)

⚠️ 내부 풀업 사용 (외부 저항 불필요)
   버튼을 누르면 LOW 신호 → FALLING edge 감지
```

### 4단계: 배선 확인

```bash
# 라즈베리파이에 SSH 접속 후 GPIO 상태 확인
ssh pi@<라즈베리파이_IP>

# GPIO 핀 상태 조회
cat /sys/class/gpio/gpiochip0/ngpio

# 현재 export된 GPIO 확인
ls /sys/class/gpio/

# 예상 출력: gpiochip0, export, unexport
```

---

## 코드 전송 및 빌드

### 5단계: PC에서 코드를 라즈베리파이로 전송

**방법 1: SCP 사용 (개별 파일 전송)**

Windows PowerShell에서:
```powershell
# rpi2 폴더 전체 전송
scp -r D:\RPI_parking_service\rpi2 pi@<라즈베리파이_IP>:/home/pi/

# 또는 개별 파일 전송
scp D:\RPI_parking_service\rpi2\*.c pi@<라즈베리파이_IP>:/home/pi/rpi2/
scp D:\RPI_parking_service\rpi2\*.h pi@<라즈베리파이_IP>:/home/pi/rpi2/
scp D:\RPI_parking_service\rpi2\Makefile pi@<라즈베리파이_IP>:/home/pi/rpi2/
```

**방법 2: VS Code Remote 사용 (권장)**

1. VS Code에서 `Remote - SSH` 확장 설치
2. Command Palette (`Ctrl+Shift+P`) → "Remote-SSH: Connect to Host"
3. `pi@<라즈베리파이_IP>` 입력
4. 폴더 선택: `/home/pi/rpi2`
5. 파일 편집 후 자동 동기화

### 6단계: 라즈베리파이에서 빌드

```bash
# 라즈베리파이에 SSH 접속
ssh pi@<라즈베리파이_IP>

# rpi2 디렉토리로 이동
cd ~/rpi2

# 빌드 (Makefile 실행)
make

# 빌드 성공 확인
ls -la gate_node

# 출력:
# -rwxr-xr-x  pi  pi  12345  May 19 10:30  gate_node
```

**빌드 에러 시:**

```bash
# 필수 패키지 재설치
sudo apt-get install -y libpaho-mqtt3-dev build-essential

# 캐시 정리 후 재빌드
make clean
make
```

---

## 테스트 1: make 빌드 성공 확인 (Task 3.3)

### 실행 방법

```bash
cd ~/rpi2

# 빌드 성공 여부 확인
make

# 예상 출력:
# gcc -Wall -pthread -c main.c -o main.o
# gcc -Wall -pthread -c gate_controller.c -o gate_controller.o
# gcc -Wall -pthread -c switch_handler.c -o switch_handler.o
# gcc -Wall -pthread -c mqtt_client.c -o mqtt_client.c
# gcc -Wall -pthread -o gate_node main.o gate_controller.o ...

# ✅ 성공하면 gate_node 실행 파일 생성됨
ls -la gate_node
```

**결과:**
- `gate_node` 파일 생성 → **Task 3.3 완료** ✅

---

## 테스트 2: 서보 모터 OPEN/CLOSED 동작 테스트 (Task 2.1)

### 사전 준비 (GPIO 권한)

```bash
# PWM 설정을 위해 /dev/mem 접근 필요
# sudo 권한으로 실행 또는 권한 설정

# 방법 1: sudo로 실행 (간단)
sudo ~/rpi2/gate_node

# 방법 2: 권한 설정 (영구적)
sudo usermod -a -G gpio pi
sudo chmod g+rw /dev/mem
# 로그아웃 후 재로그인
```

### 테스트 코드 (별도 프로그램)

간단한 테스트용 C 프로그램을 작성하는 대신, 다음 단계를 따릅니다:

```bash
# 방법 A: main.c 수정해서 테스트 (빠름)
# 또는

# 방법 B: 수동 GPIO 제어로 테스트 (권장)

# PWM 설정 (BCM 18 사용)
sudo -c "
# PWM0 export
echo 0 > /sys/class/pwm/pwmchip0/export 2>/dev/null || true

# Period 설정 (50Hz = 20000000ns)
echo 20000000 > /sys/class/pwm/pwmchip0/pwm0/period

# Duty cycle 설정 (OPEN = 500000ns)
echo 500000 > /sys/class/pwm/pwmchip0/pwm0/duty_cycle

# PWM 활성화
echo 1 > /sys/class/pwm/pwmchip0/pwm0/enable
"

# 2초 대기 (서보가 OPEN 위치로 이동)
sleep 2

# Duty cycle 변경 (CLOSED = 2400000ns)
sudo sh -c "echo 2400000 > /sys/class/pwm/pwmchip0/pwm0/duty_cycle"

# 2초 대기 (서보가 CLOSED 위치로 이동)
sleep 2

# PWM 비활성화
sudo sh -c "echo 0 > /sys/class/pwm/pwmchip0/pwm0/duty_cycle"
```

### 관찰

**OPEN 상태 (500000ns):**
- 서보 아암이 0도 방향으로 이동
- 게이트 모양이 열린 상태

**CLOSED 상태 (2400000ns):**
- 서보 아암이 180도 방향으로 이동
- 게이트 모양이 닫힌 상태

**결과:**
- 서보가 정상 동작하면 → **Task 2.1 완료** ✅
- 각도가 맞지 않으면 → config.h의 `PWM_DUTY_OPEN_NS`, `PWM_DUTY_CLOSED_NS` 값 조정

---

## 테스트 3: Switch 버튼 클릭 감지 테스트 (Task 3.1)

### 테스트 프로그램 작성

`test_switch.c` 파일 생성:

```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <poll.h>

#define GPIO_PATH "/sys/class/gpio"
#define SWITCH_PIN 23

static int export_gpio(int pin) {
  char path[256];
  snprintf(path, sizeof(path), "%s/export", GPIO_PATH);
  int fd = open(path, O_WRONLY);
  if (fd < 0) return -1;
  dprintf(fd, "%d", pin);
  close(fd);
  usleep(100000);
  return 0;
}

int main(void) {
  printf("Switch 감지 테스트 시작 (SWITCH_PIN = BCM %d)\n", SWITCH_PIN);
  printf("버튼을 누르면 이벤트가 감지됩니다. (Ctrl+C로 종료)\n\n");

  // GPIO export
  export_gpio(SWITCH_PIN);

  // Direction 설정
  char path[256];
  snprintf(path, sizeof(path), "%s/gpio%d/direction", GPIO_PATH, SWITCH_PIN);
  int fd = open(path, O_WRONLY);
  write(fd, "in", 2);
  close(fd);

  // Edge 설정
  snprintf(path, sizeof(path), "%s/gpio%d/edge", GPIO_PATH, SWITCH_PIN);
  fd = open(path, O_WRONLY);
  write(fd, "falling", 7);
  close(fd);

  // Value 파일 열기
  snprintf(path, sizeof(path), "%s/gpio%d/value", GPIO_PATH, SWITCH_PIN);
  int gpio_fd = open(path, O_RDONLY);
  if (gpio_fd < 0) {
    perror("GPIO value 파일 열기 실패");
    return -1;
  }

  struct pollfd fds[1];
  fds[0].fd = gpio_fd;
  fds[0].events = POLLPRI;

  int count = 0;
  char val[10];

  while (1) {
    // 초기 값 읽기
    lseek(gpio_fd, 0, SEEK_SET);
    read(gpio_fd, val, sizeof(val));

    // poll로 edge 대기 (timeout 60000ms = 60초)
    int ret = poll(fds, 1, 60000);

    if (ret > 0 && (fds[0].revents & POLLPRI)) {
      count++;
      lseek(gpio_fd, 0, SEEK_SET);
      read(gpio_fd, val, sizeof(val));
      printf("[%d] Switch 감지됨! (핀 상태: %c)\n", count, val[0]);
    } else if (ret == 0) {
      printf("타임아웃 (60초 이상 버튼 입력 없음)\n");
      break;
    }
  }

  close(gpio_fd);

  // Cleanup
  snprintf(path, sizeof(path), "%s/unexport", GPIO_PATH);
  fd = open(path, O_WRONLY);
  dprintf(fd, "%d", SWITCH_PIN);
  close(fd);

  printf("테스트 종료\n");
  return 0;
}
```

### 테스트 실행

```bash
cd ~/rpi2

# test_switch.c 컴파일
gcc -o test_switch test_switch.c

# 테스트 실행
sudo ./test_switch

# 출력 예:
# Switch 감지 테스트 시작 (SWITCH_PIN = BCM 23)
# 버튼을 누르면 이벤트가 감지됩니다. (Ctrl+C로 종료)
# 
# (여기서 버튼 누르기...)
# [1] Switch 감지됨! (핀 상태: 0)
# [2] Switch 감지됨! (핀 상태: 0)
```

### 관찰

- 버튼을 누를 때마다 **"Switch 감지됨!"** 메시지 출력
- 디바운싱이 정상 작동하면 연속 클릭 시에도 건너뜀

**결과:**
- Switch가 정상 감지되면 → **Task 3.1 완료** ✅

---

## 테스트 4: LED 점등/소등 테스트 (Task 3.2)

### LED 수동 제어 (sysfs)

```bash
# LED GPIO export
sudo sh -c "echo 24 > /sys/class/gpio/export"

# Direction 설정 (출력)
sudo sh -c "echo out > /sys/class/gpio/gpio24/direction"

# LED 켜기 (HIGH)
sudo sh -c "echo 1 > /sys/class/gpio/gpio24/value"
echo "LED 켜짐"
sleep 2

# LED 끄기 (LOW)
sudo sh -c "echo 0 > /sys/class/gpio/gpio24/value"
echo "LED 꺼짐"
sleep 2

# 깜빡임 테스트 (3회)
for i in {1..3}; do
  sudo sh -c "echo 1 > /sys/class/gpio/gpio24/value"
  sleep 0.3
  sudo sh -c "echo 0 > /sys/class/gpio/gpio24/value"
  sleep 0.3
done
echo "깜빡임 완료"

# Cleanup
sudo sh -c "echo 24 > /sys/class/gpio/unexport"
```

### 관찰

- 명령 실행 후 LED가 켜짐/꺼짐/깜빡임

**결과:**
- LED가 정상 작동하면 → **Task 3.2 완료** ✅

---

## 전체 통합 테스트 (1주차 완료 확인)

모든 테스트를 순서대로 진행:

```bash
# 1. 빌드 확인
cd ~/rpi2 && make

# 2. 서보 모터 테스트
# (위의 "테스트 2" 참조)

# 3. Switch 감지 테스트
sudo ./test_switch

# 4. LED 제어 테스트
# (위의 "테스트 4" 참조)
```

---

## 트러블슈팅

### 빌드 에러: "libpaho-mqtt3c not found"

```bash
sudo apt-get install -y libpaho-mqtt3-dev libpaho-mqtt3c-dev
make clean
make
```

### GPIO 권한 에러: "Permission denied"

```bash
# 방법 1: sudo 사용
sudo ./gate_node

# 방법 2: 권한 설정
sudo usermod -a -G gpio pi
sudo chmod g+rw /dev/mem
# 로그아웃 후 재로그인
```

### 서보가 움직이지 않음

- **배선 확인**: BCM 18 (GPIO 12번 핀) 신호선이 서보와 연결되었나?
- **PWM 확인**: `/sys/class/pwm/pwmchip0/` 디렉토리 존재?
- **전원 확인**: 서보 VCC가 외부 5V에 연결되었나? (라즈베리파이 5V 핀 직결 금지)
- **GND 연결**: 서보 GND와 라즈베리파이 GND가 연결되었나?

### Switch가 감지되지 않음

- **배선 확인**: BCM 23 (GPIO 16번 핀)과 GND가 연결되었나?
- **버튼 테스트**: 버튼이 정상 작동하는가? (멀티미터로 확인)
- **Edge 설정**: FALLING edge가 정상 설정되었나?

---

## 라즈베리파이 IP 찾는 방법

### 방법 1: 라우터 관리 페이지에서 확인

라우터 관리 페이지 (192.168.0.1 또는 192.168.1.1) → 연결된 장치 목록 → "raspberry"

### 방법 2: 라즈베리파이에서 직접 확인

라즈베리파이 모니터에 연결 후:
```bash
hostname -I
```

### 방법 3: nmap 사용 (PC에서)

```bash
nmap -sn 192.168.1.0/24 | grep -i raspberry
```

---

## 다음 단계 (2주차)

1주차 모든 테스트 완료 후:
- 2주차: MQTT 브로커 연결 및 전체 시나리오 통합 테스트
- `main.c` 실행: 서보 + Switch + MQTT 모두 함께 동작 확인
