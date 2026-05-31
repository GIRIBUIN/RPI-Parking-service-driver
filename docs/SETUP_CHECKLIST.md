# 시스템 설정 및 환경 구성 체크리스트

실제 RPi 환경에서 구동하기 전에 점검해야 할 사항들을 정리합니다.

---

## 🌐 네트워크 구성

### RPI3 (MQTT 브로커 호스트)
- [ ] RPI3의 IP 주소 확인
  ```bash
  # RPI3에서
  hostname -I
  ```
  **확인된 IP:** `________________`

### RPI2 (게이트 노드) - 설정 수정 필요
- [ ] `rpi2/config.h` 수정
  ```c
  #define MQTT_HOST "192.168.1.100"  // ← 실제 RPI3 IP로 변경
  ```
  **변경 예시:**
  ```c
  #define MQTT_HOST "192.168.1.50"   // RPI3의 실제 IP
  ```

### RPI1 (센싱 노드) - 설정 수정 필요
- [ ] RPI1의 MQTT 설정에서 브로커 IP를 RPI3 IP로 설정

### 네트워크 연결 확인
- [ ] RPI2에서 RPI3로 ping 가능
  ```bash
  ping <RPI3_IP>
  ```
- [ ] RPI1에서 RPI3로 ping 가능
- [ ] 각 RPI에서 mosquitto 클라이언트 설치 확인
  ```bash
  mosquitto_pub --version
  mosquitto_sub --version
  ```

---

## 🔧 각 RPI별 설정

### RPI2 (바리게이트 제어)
- [ ] libmosquitto-dev 설치
  ```bash
  sudo apt-get install libpaho-mqtt-dev
  ```
- [ ] config.h의 MQTT_HOST 수정 완료
- [ ] GPIO 권한 설정
  ```bash
  sudo usermod -a -G gpio $USER
  ```
- [ ] 빌드 성공
  ```bash
  cd rpi2 && make
  ```

### RPI3 (MQTT 브로커 + 대시보드)
- [ ] Mosquitto 설치 및 실행
  ```bash
  sudo apt-get install mosquitto
  mosquitto  # 또는 systemctl start mosquitto
  ```
- [ ] Python 의존성 설치
  ```bash
  pip install -r requirements.txt
  ```
- [ ] SQLite DB 스키마 초기화
  ```bash
  sqlite3 parking.db < schema.sql
  ```

### RPI1 (센싱 노드)
- [ ] Python 환경 설정 (필요시)
- [ ] MQTT 브로커 IP 설정

---

## 🧪 테스트 단계

### Phase 1: 개별 RPI 테스트
- [ ] RPI2 2주차 테스트 완료 (TEST_GUIDE_Week2.md 참고)
- [ ] RPI3 대시보드 수신 확인
- [ ] RPI1 센서 데이터 발행 확인

### Phase 2: 통합 테스트
- [ ] RPI1 → RPI3 MQTT 통신 확인
- [ ] RPI2 → RPI3 MQTT 통신 확인
- [ ] RPI1 주차 상태 → RPI2 게이트 제어 시나리오

### Phase 3: 시연 준비
- [ ] 물리적 모형 모두 준비 완료
- [ ] 각 센서/모터 동작 확인
- [ ] 디지털 대시보드 실시간 동기화 확인

---

## 📋 중요: MQTT_HOST 수정 기록

RPI2와 RPI1에서 MQTT 브로커 IP를 설정할 때 수정한 내용을 기록하세요.

### RPI2 (rpi2/config.h)
```
수정 날짜: __________
변경 전: 192.168.1.100
변경 후: ________________ (RPI3 실제 IP)
확인: ________________ (확인자)
```

### RPI1 (해당 설정 파일)
```
파일 경로: ________________
수정 날짜: __________
변경 전: (이전 설정)
변경 후: ________________ (RPI3 실제 IP)
확인: ________________ (확인자)
```

---

## 🔍 트러블슈팅

### MQTT 연결 실패
- [ ] RPI3에서 Mosquitto 실행 중 확인: `ps aux | grep mosquitto`
- [ ] RPI2에서 telnet으로 연결 테스트: `telnet <RPI3_IP> 1883`
- [ ] 방화벽 규칙 확인 (포트 1883 열려있는지)
- [ ] config.h의 MQTT_HOST 다시 확인

### GPIO 권한 오류
- [ ] root 권한으로 실행: `sudo ./gate_node`
- [ ] 또는 GPIO 그룹 권한 설정: `sudo usermod -a -G gpio $USER`

### 센서/모터 동작 안 함
- [ ] 회로 연결 상태 확인
- [ ] 각 GPIO 핀 번호 일치 확인 (config.h)
- [ ] 전원 공급 확인

---

## ✅ 완료 확인

모든 체크리스트가 완료되었을 때:
- [ ] 모든 점검 항목 완료
- [ ] 통합 테스트 성공
- [ ] 팀 리뷰 완료
- [ ] 시연 준비 완료

**최종 확인 날짜:** __________
**확인자:** __________
