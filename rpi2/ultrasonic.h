#ifndef ULTRASONIC_H
#define ULTRASONIC_H

// HC-SR04 초음파 센서 초기화 (TRIG: output, ECHO: input)
void ultrasonic_init(void);

// 거리 측정 (cm). 오류 또는 타임아웃 시 -1.0 반환
double ultrasonic_measure_cm(void);

// 정리 (GPIO unexport)
void ultrasonic_cleanup(void);

#endif
