#include "sys.h"
#include "delay.h"
#include "usart.h" 
#include "led.h" 		 	 
#include "lcd.h"  
#include "key.h"     
#include "malloc.h"
#include "beep.h"   
#include "touch.h"    
#include "includes.h"  

 
 
/************************************************
 ALIENTEK战舰STM32开发板实验53
 UCOSII实验3-消息队列、信号量集和软件定时器  实验 
 技术支持：www.openedv.com
 淘宝店铺：http://eboard.taobao.com 
 关注微信公众平台微信号："正点原子"，免费获取STM32资料。
 广州市星翼电子科技有限公司  
 作者：正点原子 @ALIENTEK
************************************************/


/////////////////////////UCOSII任务设置///////////////////////////////////
//START 任务
//设置任务优先级
#define START_TASK_PRIO      			10 //开始任务的优先级设置为最低
//设置任务堆栈大小
#define START_STK_SIZE  				64
//任务堆栈	
OS_STK START_TASK_STK[START_STK_SIZE];
//任务函数
void start_task(void *pdata);	
 			   
//LED任务
//设置任务优先级
#define LED_TASK_PRIO       			7 
//设置任务堆栈大小
#define LED_STK_SIZE  		    		64
//任务堆栈
OS_STK LED_TASK_STK[LED_STK_SIZE];
//任务函数
void led_task(void *pdata);

//触摸屏任务
//设置任务优先级
#define TOUCH_TASK_PRIO       		 	6
//设置任务堆栈大小
#define TOUCH_STK_SIZE  				128
//任务堆栈	
OS_STK TOUCH_TASK_STK[TOUCH_STK_SIZE];
//任务函数
void touch_task(void *pdata);

//队列消息显示任务
//设置任务优先级
#define QMSGSHOW_TASK_PRIO    			5 
//设置任务堆栈大小
#define QMSGSHOW_STK_SIZE  		 		128
//任务堆栈	
OS_STK QMSGSHOW_TASK_STK[QMSGSHOW_STK_SIZE];
//任务函数
void qmsgshow_task(void *pdata);


//主任务
//设置任务优先级
#define MAIN_TASK_PRIO       			4 
//设置任务堆栈大小
#define MAIN_STK_SIZE  					128
//任务堆栈	
OS_STK MAIN_TASK_STK[MAIN_STK_SIZE];
//任务函数
void main_task(void *pdata);

//信号量集任务
//设置任务优先级
#define FLAGS_TASK_PRIO       			3 
//设置任务堆栈大小
#define FLAGS_STK_SIZE  		 		128
//任务堆栈	
OS_STK FLAGS_TASK_STK[FLAGS_STK_SIZE];
//任务函数
void flags_task(void *pdata);


//按键扫描任务
//设置任务优先级
#define KEY_TASK_PRIO       			2 
//设置任务堆栈大小
#define KEY_STK_SIZE  					128
//任务堆栈	
OS_STK KEY_TASK_STK[KEY_STK_SIZE];
//任务函数
void key_task(void *pdata);
//////////////////////////////////////////////////////////////////////////////
    
OS_EVENT * msg_key;			//按键邮箱事件块	  
OS_EVENT * q_msg;			//消息队列
OS_TMR   * tmr1;			//软件定时器1
OS_TMR   * tmr2;			//软件定时器2
OS_TMR   * tmr3;			//软件定时器3
OS_FLAG_GRP * flags_key;	//按键信号量集
void * MsgGrp[256];			//消息队列存储地址,最大支持256个消息

//软件定时器1的回调函数	
//每100ms执行一次,用于显示CPU使用率和内存使用率		   
void tmr1_callback(OS_TMR *ptmr,void *p_arg) 
{
 	static u16 cpuusage=0;
	static u8 tcnt=0;	    
	if(tcnt==5)
	{
 		//显示CPU使用率  
		cpuusage=0;
		tcnt=0; 
	}
	cpuusage+=OSCPUUsage;
	tcnt++;				    
 	//显示内存使用率	 	  		 					    
	//显示队列当前的大小		   
 }

//软件定时器2的回调函数				  	   
void tmr2_callback(OS_TMR *ptmr,void *p_arg) 
{	
	static u8 sta=0;
	sta++;
	if(sta>6)sta=0;	 											   
}
//软件定时器3的回调函数				  	   
void tmr3_callback(OS_TMR *ptmr,void *p_arg) 
{	
	u8* p;	 
	u8 err; 
	static u8 msg_cnt=0;	//msg编号	  
	p=mymalloc(SRAMIN,13);	//申请13个字节的内存
	if(p)
	{
	 	sprintf((char*)p,"ALIENTEK %03d",msg_cnt);
		msg_cnt++;
		err=OSQPost(q_msg,p);	//发送队列
		if(err!=OS_ERR_NONE) 	//发送失败
		{
			myfree(SRAMIN,p);	//释放内存
			OSTmrStop(tmr3,OS_TMR_OPT_NONE,0,&err);	//关闭软件定时器3
 		}
	}
} 
//加载主界面   
void ucos_load_main_ui(void)
{
	delay_ms(300);
}	  


 int main(void)
 {	 		    
	delay_init();	    	 //延时函数初始化	  
  NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);//设置中断优先级分组为组2：2位抢占优先级，2位响应优先级
	uart_init(115200);	 	//串口初始化为115200

	LED_Init(LED1);
	KEY_Init(KEY1,BUTTON_MODE_GPIO);
	KEY_Init(KEY2,BUTTON_MODE_GPIO);
	KEY_Init(KEY3,BUTTON_MODE_GPIO);
	KEY_Init(KEY4,BUTTON_MODE_GPIO);	 
	BEEP_Init();
	
	ucos_load_main_ui(); 		//加载主界面
	OSInit();  	 				//初始化UCOSII
  	OSTaskCreate(start_task,(void *)0,(OS_STK *)&START_TASK_STK[START_STK_SIZE-1],START_TASK_PRIO );//创建起始任务
	OSStart();	  
}

//开始任务
void start_task(void *pdata)
{
    OS_CPU_SR cpu_sr=0;
	u8 err;	    	    
	pdata = pdata; 	
	msg_key=OSMboxCreate((void*)0);		//创建消息邮箱
	q_msg=OSQCreate(&MsgGrp[0],256);	//创建消息队列
 	flags_key=OSFlagCreate(0,&err); 	//创建信号量集		  
	  
	OSStatInit();						//初始化统计任务.这里会延时1秒钟左右	
 	OS_ENTER_CRITICAL();				//进入临界区(无法被中断打断)    
 	OSTaskCreate(led_task,(void *)0,(OS_STK*)&LED_TASK_STK[LED_STK_SIZE-1],LED_TASK_PRIO);						   
 	OSTaskCreate(touch_task,(void *)0,(OS_STK*)&TOUCH_TASK_STK[TOUCH_STK_SIZE-1],TOUCH_TASK_PRIO);	 				   
 	OSTaskCreate(qmsgshow_task,(void *)0,(OS_STK*)&QMSGSHOW_TASK_STK[QMSGSHOW_STK_SIZE-1],QMSGSHOW_TASK_PRIO);	 				   
 	OSTaskCreate(main_task,(void *)0,(OS_STK*)&MAIN_TASK_STK[MAIN_STK_SIZE-1],MAIN_TASK_PRIO);	 				   
 	OSTaskCreate(flags_task,(void *)0,(OS_STK*)&FLAGS_TASK_STK[FLAGS_STK_SIZE-1],FLAGS_TASK_PRIO);	 				   
 	OSTaskCreate(key_task,(void *)0,(OS_STK*)&KEY_TASK_STK[KEY_STK_SIZE-1],KEY_TASK_PRIO);	 				   
 	OSTaskSuspend(START_TASK_PRIO);		//挂起起始任务.
	OS_EXIT_CRITICAL();					//退出临界区(可以被中断打断)
}
//LED任务
void led_task(void *pdata)
{
	u8 t=0;
	while(1)
	{
		t++;
		delay_ms(10);
		if(t==8)LED1_STATE=1;	//LED0灭
		if(t==100)		//LED0亮
		{
			t=0;
			LED1_STATE=0;
		}
	}									 
}
//触摸屏任务
void touch_task(void *pdata)
{	  	
	while(1)
	{
		delay_ms(5);	 
	}
}     
//队列消息显示任务
void qmsgshow_task(void *pdata)
{
	u8 *p;
	u8 err;
	while(1)
	{
		p=OSQPend(q_msg,0,&err);//请求消息队列
 		myfree(SRAMIN,p);	  
		delay_ms(500);	 
	}									 
}
//主任务
void main_task(void *pdata)
{							 

	u8 err;	
 

  
 	tmr1=OSTmrCreate(10,10,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr1_callback,0,"tmr1",&err);		//100ms执行一次
	tmr2=OSTmrCreate(10,20,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr2_callback,0,"tmr2",&err);		//200ms执行一次
	tmr3=OSTmrCreate(10,10,OS_TMR_OPT_PERIODIC,(OS_TMR_CALLBACK)tmr3_callback,0,"tmr3",&err);		//100ms执行一次
	OSTmrStart(tmr1,&err);//启动软件定时器1				 
	OSTmrStart(tmr2,&err);//启动软件定时器2				 
 	while(1)
	{
		delay_ms(10);
	}
}		   
//信号量集处理任务
void flags_task(void *pdata)
{	
	
	u8 err;	    						 
	while(1)
	{
		OSFlagPend(flags_key,0X001F,OS_FLAG_WAIT_SET_ANY,0,&err);//等待信号量

		BEEP_On();
		delay_ms(50);
		BEEP_Off();
		OSFlagPost(flags_key,0X001F,OS_FLAG_CLR,&err);//全部信号量清零
 	}
}
   		    
//按键扫描任务
void key_task(void *pdata)
{	
	u8 key;		    						 
	while(1)
	{
		key=KEY_Scan();   
		if(key)OSMboxPost(msg_key,(void*)key);//发送消息
 		delay_ms(10);
	}
}
