#ifndef SWITCH_HANDLER_H
#define SWITCH_HANDLER_H

typedef void (*SwitchCallback)(void);

// Switch 인터럽트 설정
void switch_handler_init(SwitchCallback on_press);

// 정리
void switch_handler_cleanup(void);

#endif
