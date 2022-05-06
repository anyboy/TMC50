//#include "interactive.h"

#include "kernel.h"
#include <zephyr.h>
#include <stdlib.h>
#include <stdio.h>
#include "os_common_api.h"
#include "gpio.h"


char *led_thread_stack;
struct device *gpio2;
int bt_connect_flag =0;   // 0=normal   1=connect   2=disconnect 
int wakeup_disconnect_flag=0; // 0=disconnect

int state_two_num=0;
//int connect_discover_flag=1;  //开机可见可连接

extern void * app_mem_malloc(unsigned int num_bytes);
void led_process_deal(void *p1, void *p2, void *p3);


struct k_work my_device;
void interactive_start(struct k_work *work);
void gpio_init(void);
void select_flag(int flag);
void led_cntrol(void);

extern int btif_br_set_discoverable(bool enable);   //蓝牙可见，可连接
extern int btif_br_set_connnectable(bool enable);

void bt_show(bool enable)
{
    btif_br_set_discoverable(enable);
    btif_br_set_connnectable(enable);
}

void led_process_deal(void *p1, void *p2, void *p3)
{
    gpio_init();
    printk("dxk_20220406\n");
    //线程创建
    while (1)
    {
        //printk("hello\n");
        
        led_cntrol();
        os_sleep(100);  //0.1s
    }
    


}

void led_cntrol(void)
{
    //printk("bt_connect_flag =%d\n",bt_connect_flag);
    switch (bt_connect_flag)
    {
    case 1:                 //连接 常亮
        state_two_num =0;
        gpio_pin_write(gpio2, 2, 0);
        wakeup_disconnect_flag =1;
        bt_connect_flag =0;
        
        break;
    case 2:                 //断开 闪烁
        //gpio_pin_write(gpio2, 2, 1);  //灭
        state_two_num++;
        wakeup_disconnect_flag =0;
        if(state_two_num>600)
        {
            state_two_num=0;
            gpio_pin_write(gpio2, 2, 1);  //灭
            bt_connect_flag=0;
            bt_show(false);     //不可见
        }

        if((state_two_num%10)==1)
        {
            gpio_pin_write(gpio2, 2, 1);  //灭
        }else if((state_two_num%10)==5)
        {
            gpio_pin_write(gpio2, 2, 0);  //亮
        }

        break;

    default:
        state_two_num=0;
        break;
    }


}

void led_thread_creat(void)
{
    int pid;
    led_thread_stack = app_mem_malloc(1024);
   
    pid=os_thread_create(led_thread_stack,1024, led_process_deal, NULL, NULL, NULL, 8, 0, OS_NO_WAIT);

}



/*
void interactive_start(struct k_work *work)
{
    
    while(1)
    {
        select_flag(bt_connect_flag);
        printk("bt_connect_flag =%d\n",bt_connect_flag);
        os_sleep(100);
    }
}

void select_flag(int flag)
{
    switch (flag)
    {
    case 1:                 //连接 常亮
        gpio_pin_write(gpio2, 2, 0);
        bt_connect_flag =0;
        break;
    case 2:                 //断开 闪烁
        gpio_pin_write(gpio2, 2, 1);  //灭
        bt_connect_flag =0;
        break;

    default:
        break;
    }


}
*/

void gpio_init(void)
{
    gpio2 = device_get_binding(CONFIG_GPIO_ACTS_DEV_NAME);
	gpio_pin_configure(gpio2, 2, GPIO_DIR_OUT);
	gpio_pin_write(gpio2, 2, 1);	//关蓝牙灯


}

/*
void led_init(void)
{
    gpio_init();
    k_work_init(&my_device, interactive_start);

}

void led_start(void)
{
    k_work_submit(&my_device);
}*/



