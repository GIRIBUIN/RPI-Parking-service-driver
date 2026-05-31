#ifndef BUZZER_H
#define BUZZER_H

// 능동 부저 초기화
void buzzer_init(void);

// 부저 켜기
void buzzer_on(void);

// 부저 끄기
void buzzer_off(void);

// 정리 (GPIO unexport)
void buzzer_cleanup(void);

#endif
