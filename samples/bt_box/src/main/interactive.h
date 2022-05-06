#ifndef _INTERACTIVE_H_
#define _INTERACTIVE_H_

extern void led_init(void);
extern void led_start(void);

extern void led_thread_creat(void);

extern int bt_connect_flag;   // 0=normal   1=connect   2=disconnect 

#endif