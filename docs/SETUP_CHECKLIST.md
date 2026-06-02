# 시스템 설정 및 환경 구성 체크리스트

실제 RPI 환경에서 구동하기 전에 점검해야 할 항목을 정리한다.

---

## 네트워크 구성

### RPI3: MQTT Broker 호스트

- [ ] RPI3의 IP 주소 확인

```bash
hostname -I
```

확인된 IP: `________________`

### RPI1: 주차 슬롯 점유 판단 노드

- [ ] RPI1의 MQTT Broker 주소를 RPI3 IP로 설정
- [ ] RPI1에서 RPI3로 ping 가능

```bash
ping <RPI3_IP>
```

### RPI2: 입구 바리게이트 제어 노드

- [ ] `rpi2/config.h`의 MQTT Broker 주소를 RPI3 IP로 설정

```c
#define MQTT_HOST "192.168.1.50"
```

- [ ] RPI2에서 RPI3로 ping 가능

```bash
ping <RPI3_IP>
```

### MQTT 클라이언트 확인

- [ ] 각 RPI에서 Mosquitto client 설치 확인

```bash
mosquitto_pub --version
mosquitto_sub --version
```

---

## RPI별 설정

### RPI1: 주차 슬롯 점유 판단

- [ ] Python 또는 C 실행 환경 준비
- [ ] HC-SR04 Slot 1 센서 결선 확인
- [ ] HC-SR04 Slot 2 센서 결선 확인
- [ ] Slot 1 LED 결선 확인
- [ ] Slot 2 LED 결선 확인
- [ ] 두 초음파 센서 순차 측정 로직 확인
- [ ] MQTT Broker IP 설정 완료
- [ ] `parking/rpi1/slot` 발행 확인
- [ ] `parking/rpi1/distance` 발행 확인
- [ ] `parking/rpi1/lot` 발행 확인

### RPI2: 입구 바리게이트 제어

- [ ] MQTT client 라이브러리 설치

```bash
sudo apt-get install libpaho-mqtt-dev
```

- [ ] `rpi2/config.h`의 `MQTT_HOST` 수정 완료
- [ ] GPIO 권한 설정

```bash
sudo usermod -a -G gpio $USER
```

- [ ] Servo Motor open/close 각도 확인
- [ ] 입구 HC-SR04 센서 결선 확인
- [ ] 출차 Switch 결선 확인
- [ ] 만차 LED 결선 확인
- [ ] 출차 이벤트 LED/Buzzer 결선 확인
- [ ] 빌드 성공

```bash
cd rpi2
make
```

- [ ] `parking/rpi1/lot` 구독 확인
- [ ] `parking/rpi2/gate` 발행 확인
- [ ] `parking/rpi2/event` 발행 확인

### RPI3: MQTT Broker, DB, Dashboard

- [ ] Mosquitto 설치 및 실행

```bash
sudo apt-get install mosquitto
sudo systemctl start mosquitto
```

- [ ] Python 의존성 설치

```bash
pip install -r requirements.txt
```

- [ ] SQLite DB 스키마 초기화
- [ ] Dashboard 실행 확인
- [ ] 다음 토픽 구독 및 저장 확인

```text
parking/rpi1/slot
parking/rpi1/distance
parking/rpi1/lot
parking/rpi2/gate
parking/rpi2/event
```

---

## 테스트 단계

### Phase 1: 개별 RPI 테스트

- [ ] RPI1 Slot 1 거리 측정 확인
- [ ] RPI1 Slot 2 거리 측정 확인
- [ ] RPI1 `EMPTY`/`OCCUPIED` 상태 전환 확인
- [ ] RPI2 Servo Motor `OPEN`/`CLOSED` 동작 확인
- [ ] RPI2 입구 차량 접근 감지 확인
- [ ] RPI2 출차 Switch 입력 확인
- [ ] RPI3 MQTT 수신 및 DB 저장 확인

### Phase 2: MQTT 통신 테스트

- [ ] RPI1 -> RPI3 `parking/rpi1/slot` 수신 확인
- [ ] RPI1 -> RPI3 `parking/rpi1/distance` 수신 확인
- [ ] RPI1 -> RPI2/RPI3 `parking/rpi1/lot` 수신 확인
- [ ] RPI2 -> RPI3 `parking/rpi2/gate` 수신 확인
- [ ] RPI2 -> RPI3 `parking/rpi2/event` 수신 확인

### Phase 3: 통합 시나리오 테스트

- [ ] 정상 입차: 차량 접근 -> 게이트 open -> timer close -> 슬롯 `OCCUPIED` -> Dashboard 반영
- [ ] 만차: Slot 1/2 모두 `OCCUPIED` -> `FULL` -> 만차 LED/Dashboard 반영
- [ ] 만차 회차: 만차 상태에서 차량 접근 -> 게이트 open -> 출차 버튼 -> 게이트 open -> timer close
- [ ] 출차: 슬롯 `EMPTY` 전환 -> `AVAILABLE` -> 출차 버튼 -> 게이트 open -> Dashboard 반영

---

## MQTT Broker IP 수정 기록

### RPI1

```text
파일 경로: ________________
수정 날짜: ________________
변경 전: ________________
변경 후: ________________ (RPI3 실제 IP)
확인자: ________________
```

### RPI2

```text
파일 경로: rpi2/config.h
수정 날짜: ________________
변경 전: ________________
변경 후: ________________ (RPI3 실제 IP)
확인자: ________________
```

---

## 트러블슈팅

### MQTT 연결 실패

- [ ] RPI3에서 Mosquitto 실행 중인지 확인

```bash
ps aux | grep mosquitto
```

- [ ] RPI1/RPI2에서 RPI3 1883 포트 연결 확인

```bash
telnet <RPI3_IP> 1883
```

- [ ] 방화벽 규칙 확인
- [ ] MQTT Broker IP 설정 재확인

### GPIO 권한 오류

- [ ] root 권한으로 실행
- [ ] GPIO 그룹 권한 설정 후 재로그인

### 센서/모터 동작 안 함

- [ ] 회로 연결 상태 확인
- [ ] GPIO 핀 번호 확인
- [ ] 전원 공급 확인
- [ ] 초음파 센서 순차 측정 여부 확인

---

## 완료 확인

- [ ] 모든 점검 항목 완료
- [ ] 정상 입차 시나리오 성공
- [ ] 만차 시나리오 성공
- [ ] 출차 시나리오 성공
- [ ] 팀 리뷰 완료
- [ ] 시연 준비 완료

최종 확인 날짜: `________________`

확인자: `________________`
